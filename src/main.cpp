#include "FileMetaDataAnalyzer.h"
#include <iostream>
#include <iomanip>
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>

/**
 * @brief Prints the metadata key-value pairs in a formatted way.
 *
 * @tparam KeyType The type of the keys.
 * @tparam ValueType The type of the values.
 * @param metadata The metadata to be printed.
 */

template <typename KeyType, typename ValueType>
void printMetadata(const CustomMap<KeyType, ValueType>& metadata) {
    for (const auto& [key, value] : metadata) {
        std::cout << std::left << std::setw(20) << key << ": " << value << std::endl;
    }
    std::cout << std::endl;
}

void mergeMap(CustomMap<std::string, std::string>& dest, const CustomMap<std::string, std::string>& src) {
        for (const auto& [key, value] : src) {
            dest[key] = value;
        }
    }

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file_path>" << std::endl;
        return 1;
    }
    for (int i = 1; i < argc; ++i) {


        std::filesystem::path filePath = argv[i];
        CustomMap<std::string, std::string> metadata;

        // Determine file type based on file signature
        FileType fileType = determineFileType<poppler::document, std::ifstream, JPEGHeader, PNGHeader, BMPHeader, ZIPHeader, WAVHeader,GIFHeader>(filePath);

        std::cout <<"For "<<argv[i]<< " Select metadata extraction option:" << std::endl;
        std::cout << "1. Basic Metadata" << std::endl;
        std::cout << "2. Specialized Metadata" << std::endl;
        std::cout << "3. Both" << std::endl;

        int choice;
        std::cin >> choice;

        if(choice == 1 || choice == 3){
            metadata = FileMetaDataAnalyzer<BasicMetadata>::analyzeMetadata(filePath);
        }
        try{

            
            if(choice == 2 || choice == 3){
                // Analyze metadata based on file type
                switch (fileType) {
                    case FileType::PDF:
                            mergeMap(metadata, FileMetaDataAnalyzer<poppler::document>::analyzeMetadata(filePath));
                            std::cout << "PDF Metadata:" << std::endl;
                        metadata = FileMetaDataAnalyzer<poppler::document>::analyzeMetadata(filePath);
                        std::cout << "PDF Metadata:" << std::endl;
                        break;
                    case FileType::TXT:
                        mergeMap(metadata,FileMetaDataAnalyzer<std::ifstream>::analyzeMetadata(filePath));
                        std::cout << "TXT Metadata:" << std::endl;
                        break;
                    case FileType::JPEG:
                        mergeMap(metadata, FileMetaDataAnalyzer<JPEGHeader>::analyzeMetadata(filePath));
                        std::cout << "JPEG Metadata:" << std::endl;
                        break;
                    case FileType::PNG:
                        mergeMap(metadata, FileMetaDataAnalyzer<PNGHeader>::analyzeMetadata(filePath));
                        std::cout << "PNG Metadata:" << std::endl;
                        break;
                    case FileType::BMP:
                        mergeMap(metadata, FileMetaDataAnalyzer<BMPHeader>::analyzeMetadata(filePath));
                        std::cout << "BMP Metadata:" << std::endl;
                        break;
                    case FileType::ZIP:
                        mergeMap(metadata, FileMetaDataAnalyzer<ZIPHeader>::analyzeMetadata(filePath));
                        std::cout << "ZIP Metadata:" << std::endl;
                        break;
                    case FileType::WAV:
                        mergeMap(metadata, FileMetaDataAnalyzer<WAVHeader>::analyzeMetadata(filePath));
                        std::cout << "WAV Metadata:" << std::endl;
                        break;

                    case FileType::GIF:
                        mergeMap(metadata, FileMetaDataAnalyzer<GIFHeader,LogicalScreenDescriptor>::analyzeMetadata(filePath));
                        std::cout << "GIF Metadata:" << std::endl;
                        break;
                    default:
                        std::cerr << "Unsupported file format." << std::endl;
                        return 1;
                }
            }
        }catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            continue;
        }

        // Print the extracted metadata using a lambda template
        printMetadata(metadata);
    }
    return 0;
}