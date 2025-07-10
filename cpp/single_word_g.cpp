#include "language_detector_g.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <map>
#include <vector>
#include <iomanip>
#include <sstream>

// Map language codes to Lang enum values - only 6 languages
std::map<std::string, Lang> createLanguageMap() {
    return {
        {"de", Lang::De},
        {"en", Lang::En},
        {"fr", Lang::Fr},
        {"es", Lang::Es},
        {"ja", Lang::Ja},
        {"zh", Lang::Zh}
    };
}

// Map Lang enum back to string for output - only 6 languages
std::map<Lang, std::string> createReverseLanguageMap() {
    return {
        {Lang::De, "de"},
        {Lang::En, "en"},
        {Lang::Fr, "fr"},
        {Lang::Es, "es"},
        {Lang::Ja, "ja"},
        {Lang::Zh, "zh"}
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
    std::cout << "Data directory: " << dataDirectory << "\n";
    std::cout << "Testing only 6 languages: de, en, fr, es, ja, zh\n\n";
    
    try {
        // Iterate through all .txt files in the directory
        for (const auto& entry : std::filesystem::recursive_directory_iterator(dataDirectory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                std::string filename = entry.path().filename().string();
                std::string filepath = entry.path().string();
                
                // Extract language code from filename (first 2 characters)
                if (filename.length() >= 2) {
                    std::string expectedLangCode = filename.substr(0, 2);
                    
                    // Check if this language code is supported (only process our 6 languages)
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
                    } else {
                        std::cout << "Skipping " << filename << " (language " << expectedLangCode << " not in our test set)\n";
                    }
                }
            }
        }
        
        // Print overall statistics
        std::cout << "\n" << std::string(70, '=') << "\n";
        std::cout << "OVERALL RESULTS (6 languages: de, en, fr, es, ja, zh)\n";
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
