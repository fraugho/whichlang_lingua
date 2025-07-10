#include "language_detector.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <map>
#include <vector>
#include <iomanip>
#include <sstream>

// Map language codes to Lang enum values
std::map<std::string, Lang> createLanguageMap() {
    return {
        {"af", Lang::Af}, {"ar", Lang::Ar}, {"az", Lang::Az}, {"be", Lang::Be},
        {"bg", Lang::Bg}, {"bn", Lang::Bn}, {"bs", Lang::Bs}, {"ca", Lang::Ca},
        {"cs", Lang::Cs}, {"cy", Lang::Cy}, {"da", Lang::Da}, {"de", Lang::De},
        {"el", Lang::El}, {"en", Lang::En}, {"eo", Lang::Eo}, {"es", Lang::Es},
        {"et", Lang::Et}, {"eu", Lang::Eu}, {"fa", Lang::Fa}, {"fi", Lang::Fi},
        {"fr", Lang::Fr}, {"ga", Lang::Ga}, {"gu", Lang::Gu}, {"he", Lang::He},
        {"hi", Lang::Hi}, {"hr", Lang::Hr}, {"hu", Lang::Hu}, {"hy", Lang::Hy},
        {"id", Lang::Id}, {"is", Lang::Is}, {"it", Lang::It}, {"ja", Lang::Ja},
        {"ka", Lang::Ka}, {"kk", Lang::Kk}, {"ko", Lang::Ko}, {"la", Lang::La},
        {"lg", Lang::Lg}, {"lt", Lang::Lt}, {"lv", Lang::Lv}, {"mi", Lang::Mi},
        {"mk", Lang::Mk}, {"mn", Lang::Mn}, {"mr", Lang::Mr}, {"ms", Lang::Ms},
        {"nb", Lang::Nb}, {"nl", Lang::Nl}, {"nn", Lang::Nn}, {"pa", Lang::Pa},
        {"pl", Lang::Pl}, {"pt", Lang::Pt}, {"ro", Lang::Ro}, {"ru", Lang::Ru},
        {"sk", Lang::Sk}, {"sl", Lang::Sl}, {"sn", Lang::Sn}, {"so", Lang::So},
        {"sq", Lang::Sq}, {"sr", Lang::Sr}, {"st", Lang::St}, {"sv", Lang::Sv},
        {"sw", Lang::Sw}, {"ta", Lang::Ta}, {"te", Lang::Te}, {"th", Lang::Th},
        {"tl", Lang::Tl}, {"tn", Lang::Tn}, {"tr", Lang::Tr}, {"ts", Lang::Ts},
        {"uk", Lang::Uk}, {"ur", Lang::Ur}, {"vi", Lang::Vi}, {"xh", Lang::Xh},
        {"yo", Lang::Yo}, {"zh", Lang::Zh}, {"zu", Lang::Zu}
    };
}

// Map Lang enum back to string for output
std::map<Lang, std::string> createReverseLanguageMap() {
    return {
        {Lang::Af, "af"}, {Lang::Ar, "ar"}, {Lang::Az, "az"}, {Lang::Be, "be"},
        {Lang::Bg, "bg"}, {Lang::Bn, "bn"}, {Lang::Bs, "bs"}, {Lang::Ca, "ca"},
        {Lang::Cs, "cs"}, {Lang::Cy, "cy"}, {Lang::Da, "da"}, {Lang::De, "de"},
        {Lang::El, "el"}, {Lang::En, "en"}, {Lang::Eo, "eo"}, {Lang::Es, "es"},
        {Lang::Et, "et"}, {Lang::Eu, "eu"}, {Lang::Fa, "fa"}, {Lang::Fi, "fi"},
        {Lang::Fr, "fr"}, {Lang::Ga, "ga"}, {Lang::Gu, "gu"}, {Lang::He, "he"},
        {Lang::Hi, "hi"}, {Lang::Hr, "hr"}, {Lang::Hu, "hu"}, {Lang::Hy, "hy"},
        {Lang::Id, "id"}, {Lang::Is, "is"}, {Lang::It, "it"}, {Lang::Ja, "ja"},
        {Lang::Ka, "ka"}, {Lang::Kk, "kk"}, {Lang::Ko, "ko"}, {Lang::La, "la"},
        {Lang::Lg, "lg"}, {Lang::Lt, "lt"}, {Lang::Lv, "lv"}, {Lang::Mi, "mi"},
        {Lang::Mk, "mk"}, {Lang::Mn, "mn"}, {Lang::Mr, "mr"}, {Lang::Ms, "ms"},
        {Lang::Nb, "nb"}, {Lang::Nl, "nl"}, {Lang::Nn, "nn"}, {Lang::Pa, "pa"},
        {Lang::Pl, "pl"}, {Lang::Pt, "pt"}, {Lang::Ro, "ro"}, {Lang::Ru, "ru"},
        {Lang::Sk, "sk"}, {Lang::Sl, "sl"}, {Lang::Sn, "sn"}, {Lang::So, "so"},
        {Lang::Sq, "sq"}, {Lang::Sr, "sr"}, {Lang::St, "st"}, {Lang::Sv, "sv"},
        {Lang::Sw, "sw"}, {Lang::Ta, "ta"}, {Lang::Te, "te"}, {Lang::Th, "th"},
        {Lang::Tl, "tl"}, {Lang::Tn, "tn"}, {Lang::Tr, "tr"}, {Lang::Ts, "ts"},
        {Lang::Uk, "uk"}, {Lang::Ur, "ur"}, {Lang::Vi, "vi"}, {Lang::Xh, "xh"},
        {Lang::Yo, "yo"}, {Lang::Zh, "zh"}, {Lang::Zu, "zu"}
    };
}

// Helper function to trim whitespace from a string
std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

std::vector<std::string> readWordsFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filepath);
    }
    
    std::vector<std::string> words;
    std::string line;
    while (std::getline(file, line)) {
        std::string word = trim(line);
        if (!word.empty()) {
            words.push_back(word);
        }
    }
    
    return words;
}

struct TestResult {
    std::string expectedLang;
    std::string detectedLang;
    std::string word;
    std::string filename;
    int lineNumber;
    bool correct;
};

int main(int argc, char* argv[]) {
    std::string dataDirectory = "../lingua/language-testdata/single-words"; // Default directory
    
    if (argc > 1) {
        dataDirectory = argv[1];
    }
    
    auto langMap = createLanguageMap();
    auto reverseLangMap = createReverseLanguageMap();
    
    std::vector<TestResult> results;
    int totalTests = 0;
    int correctPredictions = 0;
    
    std::map<std::string, int> languageCorrect;
    std::map<std::string, int> languageTotal;
    
    std::cout << "Testing language detection accuracy on individual words...\n";
    std::cout << "Data directory: " << dataDirectory << "\n\n";
    
    try {
        // Iterate through all .txt files in the directory
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dataDirectory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                std::string filename = entry.path().filename().string();
                std::string filepath = entry.path().string();
                
                // Extract language code from filename (first 2 characters)
                if (filename.length() >= 2) {
                    std::string expectedLangCode = filename.substr(0, 2);
                    
                    // Check if this language code is supported
                    if (langMap.find(expectedLangCode) != langMap.end()) {
                        try {
                            // Read all words from file
                            std::vector<std::string> words = readWordsFromFile(filepath);
                            
                            std::cout << "Processing " << filename << " (" << words.size() << " words)...\n";
                            
                            // Test each word individually
                            for (size_t i = 0; i < words.size(); ++i) {
                                const std::string& word = words[i];
                                
                                try {
                                    // Detect language for this word
                                    Lang detectedLang = LanguageDetector::detectLanguage(word);
                                    std::string detectedLangCode = reverseLangMap[detectedLang];
                                    
                                    // Check if prediction is correct
                                    bool isCorrect = (expectedLangCode == detectedLangCode);
                                    
                                    results.push_back({
                                        expectedLangCode,
                                        detectedLangCode,
                                        word,
                                        filename,
                                        static_cast<int>(i + 1),
                                        isCorrect
                                    });
                                    
                                    totalTests++;
                                    if (isCorrect) {
                                        correctPredictions++;
                                    }
                                    
                                    // Update per-language statistics
                                    languageTotal[expectedLangCode]++;
                                    if (isCorrect) {
                                        languageCorrect[expectedLangCode]++;
                                    }
                                    
                                    // Print progress for each word (optional - comment out if too verbose)
                                    if (totalTests % 100 == 0) {
                                        std::cout << "  Processed " << totalTests << " words so far...\n";
                                    }
                                    
                                } catch (const std::exception& e) {
                                    std::cerr << "Error processing word '" << word << "' from " << filename 
                                              << " line " << (i + 1) << ": " << e.what() << "\n";
                                }
                            }
                            
                        } catch (const std::exception& e) {
                            std::cerr << "Error processing file " << filename << ": " << e.what() << "\n";
                        }
                    }
                }
            }
        }
        
        // Print overall statistics
        std::cout << "\n" << std::string(70, '=') << "\n";
        std::cout << "OVERALL RESULTS\n";
        std::cout << std::string(70, '=') << "\n";
        std::cout << "Total word tests: " << totalTests << "\n";
        std::cout << "Correct predictions: " << correctPredictions << "\n";
        std::cout << "Overall accuracy: " << std::fixed << std::setprecision(2) 
                  << (totalTests > 0 ? (100.0 * correctPredictions / totalTests) : 0.0) << "%\n\n";
        
        // Print per-language statistics
        std::cout << "PER-LANGUAGE ACCURACY\n";
        std::cout << std::string(70, '-') << "\n";
        std::cout << std::setw(8) << "Lang" << std::setw(10) << "Correct" 
                  << std::setw(10) << "Total" << std::setw(12) << "Accuracy" << "\n";
        std::cout << std::string(70, '-') << "\n";
        
        for (const auto& pair : languageTotal) {
            std::string lang = pair.first;
            int total = pair.second;
            int correct = languageCorrect[lang];
            double accuracy = total > 0 ? (100.0 * correct / total) : 0.0;
            
            std::cout << std::setw(8) << lang 
                      << std::setw(10) << correct 
                      << std::setw(10) << total 
                      << std::setw(11) << std::fixed << std::setprecision(1) << accuracy << "%\n";
        }
        
        // Print some examples of misclassifications
        std::cout << "\nMISCLASSIFICATIONS (first 20 examples):\n";
        std::cout << std::string(70, '-') << "\n";
        std::cout << std::setw(15) << "Word" << std::setw(10) << "Expected" 
                  << std::setw(10) << "Detected" << std::setw(15) << "File" 
                  << std::setw(8) << "Line" << "\n";
        std::cout << std::string(70, '-') << "\n";
        
        int errorCount = 0;
        for (const auto& result : results) {
            if (!result.correct && errorCount < 20) {
                std::cout << std::setw(15) << result.word.substr(0, 14)  // Truncate long words
                          << std::setw(10) << result.expectedLang 
                          << std::setw(10) << result.detectedLang 
                          << std::setw(15) << result.filename.substr(0, 14)  // Truncate long filenames
                          << std::setw(8) << result.lineNumber << "\n";
                errorCount++;
            }
        }
        
        // Print summary by most confused language pairs
        std::cout << "\nMOST COMMON CONFUSIONS:\n";
        std::cout << std::string(70, '-') << "\n";
        
        std::map<std::pair<std::string, std::string>, int> confusions;
        for (const auto& result : results) {
            if (!result.correct) {
                confusions[{result.expectedLang, result.detectedLang}]++;
            }
        }
        
        // Sort confusions by count
        std::vector<std::pair<std::pair<std::string, std::string>, int>> sortedConfusions(
            confusions.begin(), confusions.end());
        std::sort(sortedConfusions.begin(), sortedConfusions.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        
        std::cout << std::setw(12) << "Expected" << std::setw(12) << "Detected" 
                  << std::setw(10) << "Count" << "\n";
        std::cout << std::string(70, '-') << "\n";
        
        for (size_t i = 0; i < std::min(size_t(10), sortedConfusions.size()); ++i) {
            const auto& confusion = sortedConfusions[i];
            std::cout << std::setw(12) << confusion.first.first
                      << std::setw(12) << confusion.first.second
                      << std::setw(10) << confusion.second << "\n";
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::cerr << "Make sure the data directory exists and contains .txt files\n";
        std::cerr << "with filenames starting with 2-letter language codes.\n";
        std::cerr << "Each file should contain one word per line.\n";
        return 1;
    }
    
    return 0;
}
