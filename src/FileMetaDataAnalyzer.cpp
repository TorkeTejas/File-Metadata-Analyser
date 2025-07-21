#include "FileMetaDataAnalyzer.h"
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <type_traits>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>
#include <zip.h>
#include <type_traits>
#include <sys/stat.h>
#include <string>
#include <ctime>
#include <cassert>

BasicMetadata extractBasicMetadata(const std::filesystem::path& filePath) {
    BasicMetadata basicMetadata;

    // File name
    std::string fileName = filePath.filename().string();
    basicMetadata.fileName = fileName;

    // File size
    struct stat fileStat;
    if (stat(filePath.c_str(), &fileStat) == 0) {
        basicMetadata.fileSize = std::to_string(fileStat.st_size) + " bytes";
    }

    // File type/format
    std::string fileType = filePath.extension().string();
    basicMetadata.fileType = fileType;

    // Creation time
    std::string creationTime = std::ctime(&fileStat.st_ctime);
    basicMetadata.creationTime = creationTime;

    // Last modified time
    std::string lastModified = std::ctime(&fileStat.st_mtime);
    basicMetadata.lastModified = lastModified;

    // Last access time
    std::string lastAccess = std::ctime(&fileStat.st_atime);
    basicMetadata.lastAccess = lastAccess;

    return basicMetadata;
}

bool readGifLogicalScreenDescriptor(const std::filesystem::path& filePath, LogicalScreenDescriptor& lsd) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Read Logical Screen Descriptor
    file.read(reinterpret_cast<char*>(&lsd), sizeof(LogicalScreenDescriptor));
    file.close();

    return true;
}

void custom_assert(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Assertion failed: " << message << std::endl;
        // Additional error handling code here, like logging, reporting, etc.
        // You can also throw an exception or terminate the program if necessary
        // For demonstration, let's terminate the program
        throw std::runtime_error("Assertion failed");
    }
}

/**
 * @brief Helper function to analyze the metadata of a file based on its type.
 *
 * This function uses template specialization to handle the metadata extraction for each supported file type.
 *
 * @tparam T The file header type.
 * @param filePath The path to the file.
 * @return A `CustomMap` containing the extracted metadata.
 */
template <typename T>
CustomMap<std::string, std::string> analyzeMetadataHelper(const std::filesystem::path& filePath) {
    CustomMap<std::string, std::string> metadata ;
    std::string extension = filePath.extension().string();

    if constexpr (std::is_same_v<T, BasicMetadata>)
    {
        BasicMetadata basicMetadata = extractBasicMetadata(filePath);

        // return custom map of basic metadata;
        // store bsic metadata in custom map
        metadata["FileName"] = basicMetadata.fileName;
        metadata["FileSize"] = basicMetadata.fileSize;
        metadata["FileType"] = basicMetadata.fileType;
        metadata["CreationTime"] = basicMetadata.creationTime;
        metadata["LastModified"] = basicMetadata.lastModified;
        metadata["LastAccess"] = basicMetadata.lastAccess;
        
        return metadata;

    }
    else if constexpr (std::is_same_v<T, poppler::document>) {

        custom_assert(extension == ".pdf" , "Unexpected file extension for PDF metadata");
        // PDF metadata extraction logic
        poppler::document* doc = poppler::document::load_from_file(filePath.string());
        if (!doc || doc->is_locked()) {
            delete doc;
            return metadata;
        }

        metadata["Title"] = std::string(doc->get_title().begin(), doc->get_title().end());
        metadata["Author"] = std::string(doc->get_author().begin(), doc->get_author().end());
        metadata["Subject"] = std::string(doc->get_subject().begin(), doc->get_subject().end());
        metadata["Keywords"] = std::string(doc->get_keywords().begin(), doc->get_keywords().end());
        metadata["Creator"] = std::string(doc->get_creator().begin(), doc->get_creator().end());
        metadata["Producer"] = std::string(doc->get_producer().begin(), doc->get_producer().end());
        metadata["CreationDate"] = doc->get_creation_date();
        metadata["ModificationDate"] = doc->get_modification_date();
        metadata["FileType"] = "PDF";
        delete doc;
    } else if constexpr (std::is_same_v<T, std::ifstream>) {

        custom_assert(extension == ".txt" , "Unexpected file extension for TXT metadata");

        // TXT metadata extraction logic
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return metadata;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string fileContent = buffer.str();

        std::string line;
        std::getline(file, line);
        if (!line.empty()) {
            metadata["Title"] = line;
        }

        std::getline(file, line);
        if (!line.empty()) {
            metadata["Author"] = line;
        }

        // Extract other TXT metadata...

        metadata["FileName"] = filePath.filename().string();
        metadata["FileSize"] = std::to_string(fileContent.size()) + " bytes";
        metadata["FileType"] = "TXT";
        file.close();
    } else if constexpr (std::is_same_v<T, JPEGHeader>) {

        custom_assert(extension == ".jpg" , "Unexpected file extension for JPEG metadata");

        // JPEG metadata extraction logic
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return metadata;
        }

        JPEGHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(JPEGHeader));
        file.close();

        metadata["FileType"] = "JPEG";
        metadata["Marker"] = std::to_string(header.marker);
        metadata["Length"] = std::to_string(header.length);
        metadata["Identifier"] = std::string(reinterpret_cast<char*>(header.identifier), 5);
        metadata["Version"] = std::to_string(header.version);
        metadata["Units"] = std::to_string(static_cast<int>(header.units));
        metadata["XDensity"] = std::to_string(header.xDensity);
        metadata["YDensity"] = std::to_string(header.yDensity);
        metadata["ThumbnailWidth"] = std::to_string(static_cast<int>(header.thumbWidth));
        metadata["ThumbnailHeight"] = std::to_string(static_cast<int>(header.thumbHeight));
    } else if constexpr (std::is_same_v<T, PNGHeader>) {
        custom_assert(extension == ".png" , "Unexpected file extension for PNG metadata");

        // PNG metadata extraction logic
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return metadata;
        }

        PNGHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(PNGHeader));
        file.close();

        metadata["FileType"] = "PNG";
        metadata["Signature"] = std::string(reinterpret_cast<char*>(header.signature), 8);
        metadata["Width"] = std::to_string(header.width);
        metadata["Height"] = std::to_string(header.height);
    } else if constexpr (std::is_same_v<T, BMPHeader>) {

        custom_assert(extension == ".bmp" , "Unexpected file extension for BMP metadata");

    // BMP metadata extraction logic
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return metadata;
        }

        BMPHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(BMPHeader));
        file.close();

        metadata["FileType"] = "BMP";
        metadata["Signature"] = std::string(header.signature, 2);
        metadata["FileSize"] = std::to_string(header.fileSize);
        metadata["Width"] = std::to_string(header.width);
        metadata["Height"] = std::to_string(header.height);
    } else if constexpr (std::is_same_v<T, ZIPHeader>) {

        custom_assert(extension == ".zip" , "Unexpected file extension for ZIP metadata");

        // ZIP metadata extraction logic
        int error;
        zip_t* zip = zip_open(filePath.string().c_str(), 0, &error);
        if (!zip) {
            // Handle the error code in `error`
            return metadata;
        }

        // Get the overall ZIP file information
        zip_stat_t zipStat;
        if (zip_stat(zip, "", 0, &zipStat) == 0) {
            metadata["FileType"] = "ZIP";
            metadata["FileSize"] = std::to_string(zipStat.size) + " bytes";
        }

        // Get the ZIP archive comment
        int commentLength;
        const char* comment = zip_get_archive_comment(zip, &commentLength, 0);
        if (commentLength > 0) {
            metadata["Comment"] = std::string(comment, commentLength);
        }

        // Get the first file in the ZIP archive
        zip_file_t* file = zip_fopen_index(zip, 0, 0);
        if (file) {
            ZIPHeader header;
            zip_fread(file, &header, sizeof(ZIPHeader));

            // Extract metadata from the ZIP header
            metadata["FileName"] = std::string(reinterpret_cast<char*>(header.fileNameLength + 1), header.fileNameLength);
            metadata["CompressedSize"] = std::to_string(header.compressedSize) + " bytes";
            metadata["CompressionMethod"] = std::to_string(header.compressionMethod);
            metadata["LastModificationTime"] = std::to_string(header.lastModTime);
            metadata["LastModificationDate"] = std::to_string(header.lastModDate);
            metadata["CRC32"] = std::to_string(header.crc32);
            metadata["UncompressedSize"] = std::to_string(header.uncompressedSize) + " bytes";

            zip_fclose(file);
        }

        zip_close(zip);
    } else if constexpr (std::is_same_v<T, WAVHeader>) {

        custom_assert(extension == ".wav" , "Unexpected file extension for WAV metadata");

        //WAV metadata extraction logic
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return metadata;
        }

        WAVHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(WAVHeader));
        file.close();

        metadata["FileType"] = "WAV";
        metadata["RIFFTag"] = std::string(header.riffTag, 4);
        metadata["RIFFSize"] = std::to_string(header.riffSize);
        metadata["WAVETag"] = std::string(header.waveTag, 4);
        metadata["FMTTag"] = std::string(header.fmtTag, 4);
        metadata["FMTSize"] = std::to_string(header.fmtSize);
        metadata["AudioFormat"] = std::to_string(header.audioFormat);
        metadata["NumChannels"] = std::to_string(header.numChannels);
        metadata["SampleRate"] = std::to_string(header.sampleRate);
        metadata["ByteRate"] = std::to_string(header.byteRate);
        metadata["BlockAlign"] = std::to_string(header.blockAlign);
        metadata["BitsPerSample"] = std::to_string(header.bitsPerSample);
        metadata["DataTag"] = std::string(header.dataTag, 4);
        metadata["DataSize"] = std::to_string(header.dataSize);
    }else if constexpr (std::is_same_v<T, GIFHeader>) {
        custom_assert(extension == ".gif" , "Unexpected file extension for GIF metadata");

        // GIF metadata extraction logic
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return metadata;
        }

        GIFHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(GIFHeader));
        file.close();

        metadata["FileType"] = "GIF";
        metadata["Signature"] = std::string(header.signature, 3);
        metadata["Version"] = std::string(header.version, 3);
        // Include additional metadata extraction for GIF files if needed
    } else if constexpr (std::is_same_v<T, LogicalScreenDescriptor>) {

        custom_assert(extension == ".gif" , "Unexpected file extension for GIF metadata");
        // GIF metadata extraction logic
        LogicalScreenDescriptor lsd;
        if (!readGifLogicalScreenDescriptor(filePath, lsd)) {
            return metadata;
        }

        metadata["FileType"] = "GIF";
        metadata["Width"] = std::to_string(lsd.width);
        metadata["Height"] = std::to_string(lsd.height);
        metadata["PackedFields"] = std::to_string(lsd.packedFields);
        metadata["BackgroundColorIndex"] = std::to_string(lsd.backgroundColorIndex);
        metadata["PixelAspectRatio"] = std::to_string(lsd.pixelAspectRatio);
    }

    return metadata;
}

/**
 * @brief Determines the file type based on the file signature.
 *
 * This function uses a fold expression to check the file signature and return the corresponding `FileType`.
 *
 * @tparam T The supported file header types.
 * @param filePath The path to the file.
 * @return The determined file type.
 */
template <typename... T>
requires (sizeof...(T) > 0)
FileType determineFileType(const std::filesystem::path& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return FileType::UNKNOWN;
    }

    uint8_t signature[8];
    file.read(reinterpret_cast<char*>(signature), sizeof(signature));

    // Fold expression to check the file signature
    return (
        [&]() -> FileType {
            return (
                (std::memcmp(signature, JPEGSignature, sizeof(JPEGSignature)) == 0) ? FileType::JPEG :
                (std::memcmp(signature, PNGSignature, sizeof(PNGSignature)) == 0) ? FileType::PNG :
                (std::memcmp(signature, BMPSignature, sizeof(BMPSignature)) == 0) ? FileType::BMP :
                (signature[0] == '%' && signature[1] == 'P' && signature[2] == 'D' && signature[3] == 'F') ? FileType::PDF :
                (std::memcmp(signature, ZIPSignature, sizeof(ZIPSignature)) == 0) ? FileType::ZIP :
                (std::memcmp(signature, WAVSignature, sizeof(WAVSignature)) == 0) ? FileType::WAV :
                (std::memcmp(signature, GIFSignature, sizeof(GIFSignature)) == 0) ? FileType::GIF : // Add GIF detection
                FileType::TXT
            );
        }()
    );
}


// Explicit template instantiations for the supported file header types
template FileType determineFileType<poppler::document, std::ifstream, JPEGHeader, PNGHeader, BMPHeader, ZIPHeader, WAVHeader,GIFHeader>(const std::filesystem::path& filePath);


// Explicit template instantiations for the supported file header types
// template FileType determineFileType<poppler::document, std::ifstream, JPEGHeader, PNGHeader, BMPHeader, ZIPHeader, WAVHeader>(const std::filesystem::path& filePath);

// Explicit template instantiations for the FileMetaDataAnalyzer class
template CustomMap<std::string, std::string> FileMetaDataAnalyzer<poppler::document>::analyzeMetadata(const std::filesystem::path& filePath);
template CustomMap<std::string, std::string> FileMetaDataAnalyzer<std::ifstream>::analyzeMetadata(const std::filesystem::path& filePath);
template CustomMap<std::string, std::string> FileMetaDataAnalyzer<JPEGHeader>::analyzeMetadata(const std::filesystem::path& filePath);
template CustomMap<std::string, std::string> FileMetaDataAnalyzer<PNGHeader>::analyzeMetadata(const std::filesystem::path& filePath);
template CustomMap<std::string, std::string> FileMetaDataAnalyzer<BMPHeader>::analyzeMetadata(const std::filesystem::path& filePath);
template CustomMap<std::string, std::string> FileMetaDataAnalyzer<ZIPHeader>::analyzeMetadata(const std::filesystem::path& filePath);
template CustomMap<std::string, std::string> FileMetaDataAnalyzer<WAVHeader>::analyzeMetadata(const std::filesystem::path& filePath);
template CustomMap<std::string, std::string> FileMetaDataAnalyzer<GIFHeader>::analyzeMetadata(const std::filesystem::path& filePath);
template CustomMap<std::string, std::string> FileMetaDataAnalyzer<LogicalScreenDescriptor>::analyzeMetadata(const std::filesystem::path& filePath);
template CustomMap<std::string, std::string> FileMetaDataAnalyzer<BasicMetadata>::analyzeMetadata(const std::filesystem::path& filePath);
