#ifndef LANGUAGE_DETECTOR_HPP
#define LANGUAGE_DETECTOR_HPP

#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <functional>
#include <cctype>
#include <cstdint>
#include "weights.hpp"

class LanguageDetector {
private:
    static constexpr size_t DIMENSION = 1 << 12;  // 4096
    static constexpr uint32_t BIGRAM_MASK = (1 << 16) - 1;
    static constexpr uint32_t TRIGRAM_MASK = (1 << 24) - 1;
    static constexpr uint32_t SEED = 3242157231u;
    
    // Japanese and CJK Unicode ranges
    static constexpr uint32_t JP_PUNCT_START = 0x3000;
    static constexpr uint32_t JP_PUNCT_END = 0x303f;
    static constexpr uint32_t JP_HIRAGANA_START = 0x3040;
    static constexpr uint32_t JP_HIRAGANA_END = 0x309f;
    static constexpr uint32_t JP_KATAKANA_START = 0x30a0;
    static constexpr uint32_t JP_KATAKANA_END = 0x30ff;
    static constexpr uint32_t CJK_KANJI_START = 0x4e00;
    static constexpr uint32_t CJK_KANJI_END = 0x9faf;
    static constexpr uint32_t JP_HALFWIDTH_KATAKANA_START = 0xff61;
    static constexpr uint32_t JP_HALFWIDTH_KATAKANA_END = 0xff90;
    
    enum class Feature {
        AsciiNGram,
        Unicode,
        UnicodeClass
    };
    
    struct FeatureToken {
        Feature type;
        uint32_t value;
        
        FeatureToken(Feature t, uint32_t v) : type(t), value(v) {}
    };
    
    static uint32_t murmurhash2(uint32_t k, uint32_t seed) {
        constexpr uint32_t M = 0x5bd1e995;
        uint32_t h = seed;
        
        k *= M;
        k ^= k >> 24;
        k *= M;
        h *= M;
        h ^= k;
        h ^= h >> 13;
        h *= M;
        h ^= h >> 15;
        
        return h;
    }
    
    static uint32_t featureToHash(const FeatureToken& token) {
        switch (token.type) {
            case Feature::AsciiNGram:
                return murmurhash2(token.value, SEED);
            case Feature::Unicode:
                return murmurhash2(token.value / 128, SEED ^ 2);
            case Feature::UnicodeClass:
                return murmurhash2(token.value, SEED ^ 4);
        }
        return 0;
    }
    
    static uint32_t classifyCodepoint(char32_t chr) {
        static const std::vector<uint32_t> CLASSIFICATION_POINTS = {
            160, 161, 171, 172, 173, 174, 187, 192, 196, 199, 200, 201, 202, 205,
            214, 220, 223, 224, 225, 226, 227, 228, 231, 232, 233, 234, 235, 236,
            237, 238, 239, 242, 243, 244, 245, 246, 249, 250, 251, 252, 333, 339,
            JP_PUNCT_START, JP_PUNCT_END, JP_HIRAGANA_START, JP_HIRAGANA_END,
            JP_KATAKANA_START, JP_KATAKANA_END, CJK_KANJI_START, CJK_KANJI_END,
            JP_HALFWIDTH_KATAKANA_START, JP_HALFWIDTH_KATAKANA_END
        };
        
        auto it = std::lower_bound(CLASSIFICATION_POINTS.begin(), CLASSIFICATION_POINTS.end(), 
                                   static_cast<uint32_t>(chr));
        return static_cast<uint32_t>(std::distance(CLASSIFICATION_POINTS.begin(), it));
    }
    
    static bool isAscii(char32_t c) {
        return c <= 127;
    }
    
    static bool isAlphaNumeric(char32_t c) {
        return std::isalnum(static_cast<int>(c)) || (!isAscii(c));
    }
    
    static char32_t toLowerAscii(char32_t c) {
        if (isAscii(c)) {
            return std::tolower(static_cast<int>(c));
        }
        return c;
    }
    
    // Simple UTF-8 to UTF-32 conversion
    static std::u32string utf8ToUtf32(const std::string& utf8) {
        std::u32string result;
        
        for (size_t i = 0; i < utf8.length();) {
            char32_t codepoint = 0;
            unsigned char c = utf8[i];
            
            if (c <= 0x7F) {
                // 1-byte character
                codepoint = c;
                i += 1;
            } else if ((c & 0xE0) == 0xC0) {
                // 2-byte character
                if (i + 1 < utf8.length()) {
                    codepoint = ((c & 0x1F) << 6) | (utf8[i + 1] & 0x3F);
                    i += 2;
                } else break;
            } else if ((c & 0xF0) == 0xE0) {
                // 3-byte character
                if (i + 2 < utf8.length()) {
                    codepoint = ((c & 0x0F) << 12) | 
                               ((utf8[i + 1] & 0x3F) << 6) | 
                               (utf8[i + 2] & 0x3F);
                    i += 3;
                } else break;
            } else if ((c & 0xF8) == 0xF0) {
                // 4-byte character
                if (i + 3 < utf8.length()) {
                    codepoint = ((c & 0x07) << 18) | 
                               ((utf8[i + 1] & 0x3F) << 12) | 
                               ((utf8[i + 2] & 0x3F) << 6) | 
                               (utf8[i + 3] & 0x3F);
                    i += 4;
                } else break;
            } else {
                // Invalid UTF-8, skip
                i += 1;
                continue;
            }
            
            result.push_back(codepoint);
        }
        
        return result;
    }
    
    static void emitTokens(const std::string& text, std::function<void(const FeatureToken&)> listener) {
        std::u32string utf32Text = utf8ToUtf32(text);
        
        uint32_t prev = static_cast<uint32_t>(' ');
        int numPreviousAsciiChr = 1;
        
        for (char32_t chr : utf32Text) {
            uint32_t code = toLowerAscii(chr);
            
            if (!isAscii(chr)) {
                listener(FeatureToken(Feature::Unicode, static_cast<uint32_t>(chr)));
                listener(FeatureToken(Feature::UnicodeClass, classifyCodepoint(chr)));
                numPreviousAsciiChr = 0;
                continue;
            }
            
            prev = (prev << 8) | code;
            
            switch (numPreviousAsciiChr) {
                case 0:
                    numPreviousAsciiChr = 1;
                    break;
                case 1:
                    listener(FeatureToken(Feature::AsciiNGram, prev & BIGRAM_MASK));
                    numPreviousAsciiChr = 2;
                    break;
                case 2:
                    listener(FeatureToken(Feature::AsciiNGram, prev & BIGRAM_MASK));
                    listener(FeatureToken(Feature::AsciiNGram, prev & TRIGRAM_MASK));
                    numPreviousAsciiChr = 3;
                    break;
                case 3:
                    listener(FeatureToken(Feature::AsciiNGram, prev & BIGRAM_MASK));
                    listener(FeatureToken(Feature::AsciiNGram, prev & TRIGRAM_MASK));
                    listener(FeatureToken(Feature::AsciiNGram, prev));
                    break;
            }
            
            if (!isAlphaNumeric(chr)) {
                prev = static_cast<uint32_t>(' ');
            }
        }
    }

public:
    static Lang detectLanguage(const std::string& text) {
        const size_t numLanguages = LANGUAGES.size();
        std::vector<float> scores(numLanguages, 0.0f);
        uint32_t numFeatures = 0;
        
        emitTokens(text, [&](const FeatureToken& token) {
            numFeatures++;
            uint32_t bucket = featureToHash(token) % DIMENSION;
            size_t idx = bucket * numLanguages;
            
            for (size_t i = 0; i < numLanguages; ++i) {
                scores[i] += WEIGHTS[idx + i];
            }
        });
        
        if (numFeatures == 0) {
            // Default to English
            return Lang::Eng;
        }
        
        float sqrtInvNumFeatures = 1.0f / std::sqrt(static_cast<float>(numFeatures));
        for (size_t i = 0; i < numLanguages; ++i) {
            scores[i] = scores[i] * sqrtInvNumFeatures + INTERCEPTS[i];
        }
        
        auto maxIt = std::max_element(scores.begin(), scores.end());
        size_t langId = std::distance(scores.begin(), maxIt);
        
        return LANGUAGES[langId];
    }
};

#endif // LANGUAGE_DETECTOR_HPP
