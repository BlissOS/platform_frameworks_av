/*
 * Copyright (C) 2014 The CyanogenMod Project
 * Copyright (C) 2021 The Android-x86 Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "FFMPEGUtil"
#include <utils/Log.h>

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/FFMPEGUtil.h>

namespace android {

enum MetaKeyType{
    INT32, INT64, STRING, DATA, CSD
};

struct MetaKeyEntry{
    int MetaKey;
    const char* MsgKey;
    MetaKeyType KeyType;
};

static const MetaKeyEntry MetaKeyTable[] {
   {kKeyAACAOT               , "aac-profile"            , INT32},
   {kKeyArbitraryMode        , "use-arbitrary-mode"     , INT32},
   {kKeyBitsPerRawSample     , "bits-per-raw-sample"    , INT32},
   {kKeyBitRate              , "bitrate"                , INT32},
   {kKeyBlockAlign           , "block-align"            , INT32},
   {kKeyChannelCount         , "channel-count"          , INT32},
   {kKeyCodecId              , "codec-id"               , INT32},
   {kKeyCodedSampleBits      , "coded-sample-bits"      , INT32},
   {kKeyFileFormat           , "file-format"            , INT32},
   {kKeyRawCodecData         , "raw-codec-data"         , DATA},
   {kKeyRawCodecSpecificData , "raw-codec-specific-data", CSD},
   {kKeyPcmEncoding          , "pcm-encoding"           , INT32},
   {kKeyRVVersion            , "rv-version"             , INT32},
   {kKeySampleFormat         , "sample-format"          , INT32},
   {kKeySampleRate           , "sample-rate"            , INT32},
   {kKeyWMAVersion           , "wma-version"            , INT32},  // int32_t
   {kKeyWMVVersion           , "wmv-version"            , INT32},
   {kKeyDivXVersion          , "divx-version"           , INT32},
   {kKeyThumbnailTime        , "thumbnail-time"         , INT64},
};

const char* FFMPEGUtil::getMsgKey(int key) {
    static const size_t numMetaKeys =
                     sizeof(MetaKeyTable) / sizeof(MetaKeyTable[0]);
    size_t i;
    for (i = 0; i < numMetaKeys; ++i) {
        if (key == MetaKeyTable[i].MetaKey) {
            return MetaKeyTable[i].MsgKey;
        }
    }
    return "unknown";
}

void FFMPEGUtil::convertMetaDataToMessageFF(
        const MetaDataBase *meta, sp<AMessage> *format) {
    const char * str_val;
    int32_t int32_val;
    int64_t int64_val;
    uint32_t data_type;
    const void * data;
    size_t size;
    static const size_t numMetaKeys =
                     sizeof(MetaKeyTable) / sizeof(MetaKeyTable[0]);
    size_t i;
    for (i = 0; i < numMetaKeys; ++i) {
        if (MetaKeyTable[i].KeyType == INT32 &&
            meta->findInt32(MetaKeyTable[i].MetaKey, &int32_val)) {
            ALOGV("found metakey %s of type int32", MetaKeyTable[i].MsgKey);
            format->get()->setInt32(MetaKeyTable[i].MsgKey, int32_val);
        } else if (MetaKeyTable[i].KeyType == INT64 &&
                 meta->findInt64(MetaKeyTable[i].MetaKey, &int64_val)) {
            ALOGV("found metakey %s of type int64", MetaKeyTable[i].MsgKey);
            format->get()->setInt64(MetaKeyTable[i].MsgKey, int64_val);
        } else if (MetaKeyTable[i].KeyType == STRING &&
                 meta->findCString(MetaKeyTable[i].MetaKey, &str_val)) {
            ALOGV("found metakey %s of type string", MetaKeyTable[i].MsgKey);
            format->get()->setString(MetaKeyTable[i].MsgKey, str_val);
        } else if ( (MetaKeyTable[i].KeyType == DATA ||
                   MetaKeyTable[i].KeyType == CSD) &&
                   meta->findData(MetaKeyTable[i].MetaKey, &data_type, &data, &size)) {
            ALOGV("found metakey %s of type data", MetaKeyTable[i].MsgKey);
            if (MetaKeyTable[i].KeyType == CSD) {
                const char *mime;
                CHECK(meta->findCString(kKeyMIMEType, &mime));
                if (strcasecmp(mime, MEDIA_MIMETYPE_VIDEO_AVC)) {
                    sp<ABuffer> buffer = new ABuffer(size);
                    memcpy(buffer->data(), data, size);
                    buffer->meta()->setInt32("csd", true);
                    buffer->meta()->setInt64("timeUs", 0);
                    format->get()->setBuffer("csd-0", buffer);
                } else {
                    const uint8_t *ptr = (const uint8_t *)data;
                    CHECK(size >= 8);
                    int seqLength = 0, picLength = 0;
                    for (size_t i = 4; i < (size - 4); i++)
                    {
                        if ((*(ptr + i) == 0) && (*(ptr + i + 1) == 0) &&
                           (*(ptr + i + 2) == 0) && (*(ptr + i + 3) == 1))
                            seqLength = i;
                    }
                    sp<ABuffer> buffer = new ABuffer(seqLength);
                    memcpy(buffer->data(), data, seqLength);
                    buffer->meta()->setInt32("csd", true);
                    buffer->meta()->setInt64("timeUs", 0);
                    format->get()->setBuffer("csd-0", buffer);
                    picLength=size-seqLength;
                    sp<ABuffer> buffer1 = new ABuffer(picLength);
                    memcpy(buffer1->data(), (const uint8_t *)data + seqLength, picLength);
                    buffer1->meta()->setInt32("csd", true);
                    buffer1->meta()->setInt64("timeUs", 0);
                    format->get()->setBuffer("csd-1", buffer1);
                }
            } else {
                sp<ABuffer> buffer = new ABuffer(size);
                memcpy(buffer->data(), data, size);
                format->get()->setBuffer(MetaKeyTable[i].MsgKey, buffer);
            }
        }
    }
}

void FFMPEGUtil::convertMessageToMetaDataFF(
        const sp<AMessage> &msg, sp<MetaData> &meta) {
    AString str_val;
    int32_t int32_val;
    int64_t int64_val;
    static const size_t numMetaKeys =
                     sizeof(MetaKeyTable) / sizeof(MetaKeyTable[0]);
    size_t i;
    for (i = 0; i < numMetaKeys; ++i) {
        if (MetaKeyTable[i].KeyType == INT32 &&
                msg->findInt32(MetaKeyTable[i].MsgKey, &int32_val)) {
            ALOGV("found metakey %s of type int32", MetaKeyTable[i].MsgKey);
            meta->setInt32(MetaKeyTable[i].MetaKey, int32_val);
        } else if (MetaKeyTable[i].KeyType == INT64 &&
                msg->findInt64(MetaKeyTable[i].MsgKey, &int64_val)) {
            ALOGV("found metakey %s of type int64", MetaKeyTable[i].MsgKey);
            meta->setInt64(MetaKeyTable[i].MetaKey, int64_val);
        } else if (MetaKeyTable[i].KeyType == STRING &&
                msg->findString(MetaKeyTable[i].MsgKey, &str_val)) {
            ALOGV("found metakey %s of type string", MetaKeyTable[i].MsgKey);
            meta->setCString(MetaKeyTable[i].MetaKey, str_val.c_str());
        }
    }
}

}
