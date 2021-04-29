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
#ifndef FFMPEG_UTIL_H
#define FFMPEG_UTIL_H

#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/MetaData.h>

namespace android {

struct FFMPEGUtil {

    static void convertMessageToMetaDataFF(
            const sp<AMessage> &msg, sp<MetaData> &meta);

    static void convertMetaDataToMessageFF(
        const MetaDataBase *meta, sp<AMessage> *format);

    static const char* getMsgKey(int key);

};

}

#endif
