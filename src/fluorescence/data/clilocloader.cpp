
#include "clilocloader.hpp"

#include <unicode/regex.h>

#include <net/packetreader.hpp>
#include <misc/log.hpp>

namespace fluo {
namespace data {

ClilocLoader::ClilocLoader() : bufferSize_(0), buffer_(NULL) {
}

ClilocLoader::~ClilocLoader() {
    if (buffer_) {
        free(buffer_);
        buffer_ = NULL;
    }
}

UnicodeString ClilocLoader::get(unsigned int id) {
    std::map<unsigned int, ClilocEntry>::iterator iter = entryMap_.find(id);
    if (iter != entryMap_.end()) {
        if (iter->second.useCount_ == USE_COUNT_CACHE_LIMIT) {
            // cache it now
            iter->second.useCount_ = USE_COUNT_CACHE_LIMIT + 1;
            iter->second.cachedString_ = readEntry(iter->second);
            return iter->second.cachedString_;
        } else if (iter->second.useCount_ > USE_COUNT_CACHE_LIMIT) {
            // already cached
            return iter->second.cachedString_;
        } else {
            ++iter->second.useCount_;
            return readEntry(iter->second);
        }
    }

    return UnicodeString("##CLILOCERROR");
}

UnicodeString ClilocLoader::get(unsigned int id, const UnicodeString& paramString) {
    static UnicodeString params[9];
    static UnicodeString paramRegex[9] = {
        "~1_[^~]+~", "~2_[^~]+~", "~3_[^~]+~", "~4_[^~]+~", "~5_[^~]+~",
        "~6_[^~]+~", "~7_[^~]+~", "~8_[^~]+~", "~9_[^~]+~",
    };

    UnicodeString str = get(id);

    if (paramString.length() > 0) {
        UErrorCode status = U_ZERO_ERROR;
        RegexMatcher paramSplitter("\\t", 0, status);
        unsigned int paramCount = paramSplitter.split(paramString, params, 9, status);

        //LOG_DEBUG << "param count after split: " << paramCount << std::endl;
        //LOG_DEBUG << "str before replace: " << str << std::endl;

        for (unsigned int i = 0; i < paramCount; ++i) {
            //LOG_DEBUG << "Param is \"" << params[i] << "\"" << std::endl;
            //status = U_ZERO_ERROR;
            RegexMatcher curParamMatcher(paramRegex[i], str, 0, status);
            //if (status != U_ZERO_ERROR) {
                //LOG_DEBUG << "clilocloader ustatus=" << status << std::endl;
            //}

            //status = U_ZERO_ERROR;

            str = curParamMatcher.replaceAll(params[i], status);
            //if (status != U_ZERO_ERROR) {
                //LOG_DEBUG << "clilocloader ustatus=" << status << std::endl;
            //}
            //LOG_DEBUG << "str after replace: " << str << std::endl;
        }
    }

    return str;
}

void ClilocLoader::indexFile(const boost::filesystem::path& path, bool isEnu) {
    unsigned int startBufferSize = bufferSize_;

    boost::filesystem::ifstream* stream = isEnu ? &enuStream_ : &languageStream_;
    stream->open(path);

    unsigned int filesize = boost::filesystem::file_size(path);

    stream->seekg(6);
    uint8_t indexBuffer[7];
    uint32_t id;
    uint16_t len;

    while (stream->tellg() != (std::streampos)filesize) {
        if (stream->tellg() == (std::streampos)-1) {
            LOG_ERROR << "Received value -1 from stream->tellg() in ClilocLoader::indexFile" << std::endl;
            break;
        }
        stream->read(reinterpret_cast<char*>(indexBuffer), 7);
        ClilocEntry curEntry;

        id = indexBuffer[0];
        id |= ((uint32_t)indexBuffer[1]) << 8;
        id |= ((uint32_t)indexBuffer[2]) << 16;
        id |= ((uint32_t)indexBuffer[3]) << 24;

        len = indexBuffer[5];
        len |= ((uint16_t)indexBuffer[6]) << 8;

        curEntry.id_ = id;
        curEntry.len_ = len;
        curEntry.streampos_ = stream->tellg();
        curEntry.fromEnu_ = isEnu;

        if (len > 0) {
            entryMap_[curEntry.id_] = curEntry;

            //LOG_DEBUG << "filesize: " << filesize << " cur=" << stream->tellg() << " len=" << curEntry.len_ << std::hex << " iB[5]=" << (unsigned int)indexBuffer[5] << " iB[6]=" << (unsigned int)indexBuffer[6] << std::dec << std::endl;

            stream->seekg(curEntry.len_, std::ios::cur);
            bufferSize_ = (std::max)(bufferSize_, curEntry.len_);
        }
    }

    if (bufferSize_ > startBufferSize) {
        buffer_ = reinterpret_cast<int8_t*>(realloc(buffer_, bufferSize_));
    }
}

UnicodeString ClilocLoader::readEntry(const ClilocEntry& entry) {
    boost::filesystem::ifstream* stream = entry.fromEnu_ ? &enuStream_ : &languageStream_;

    stream->seekg(entry.streampos_);
    stream->read(reinterpret_cast<char*>(buffer_), entry.len_);

    unsigned int indexHelper = 0;
    UnicodeString ret;
    net::PacketReader::readUtf8Fixed(buffer_, entry.len_, indexHelper, ret, entry.len_);

    unsigned int x = 0;
    ++x;
    x = ret.length();

    return ret;
}


}
}
