/*
 * Copyright 2021 The Android Open Source Project
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

#pragma once

#include <android-base/thread_annotations.h>
#include <android/gui/IHdrLayerInfoListener.h>
#include <binder/IBinder.h>

#include <unordered_map>

#include "WpHash.h"

namespace android {

class HdrLayerInfoReporter final : public IBinder::DeathRecipient {
public:
    struct HdrLayerInfo {
        int32_t numberOfHdrLayers = 0;
        int32_t maxW = 0;
        int32_t maxH = 0;
        int32_t flags = 0;
        // Counter-intuitively a value of "1" means "as much as you can give me" due to "1" being
        // the default value for all layers, so any HDR layer with a value of 1.f means no
        // reduced maximum has been requested
        // TODO: Should the max desired ratio have a better meaning for HLG/PQ so this can be
        // eliminated? If we assume an SDR white point of even just 100 nits for those content
        // then HLG could have a meaningful max ratio of 10.f and PQ of 100.f instead of needing
        // to treat 1.f as "uncapped"
        // With peak display brightnesses exceeding 1,000 nits currently, HLG's request could
        // actually be satisfied in some ambient conditions such that limiting that max for that
        // content in theory makes sense
        float maxDesiredSdrHdrRatio = 0.f;

        bool operator==(const HdrLayerInfo& other) const {
            return numberOfHdrLayers == other.numberOfHdrLayers && maxW == other.maxW &&
                    maxH == other.maxH && flags == other.flags;
        }

        bool operator!=(const HdrLayerInfo& other) const { return !(*this == other); }

        void mergeDesiredRatio(float update) {
            if (maxDesiredSdrHdrRatio == 0.f) {
                // If nothing is set, take the incoming value
                maxDesiredSdrHdrRatio = update;
            } else if (update == 1.f) {
                // If the request is to "go to max", then take it regardless
                maxDesiredSdrHdrRatio = 1.f;
            } else if (maxDesiredSdrHdrRatio != 1.f) {
                // If we're not currently asked to "go to max", then take the max
                // of the incoming requests
                maxDesiredSdrHdrRatio = std::max(maxDesiredSdrHdrRatio, update);
            }
        }
    };

    HdrLayerInfoReporter() = default;
    ~HdrLayerInfoReporter() final = default;

    // Dispatches updated layer fps values for the registered listeners
    // This method promotes Layer weak pointers and performs layer stack traversals, so mStateLock
    // must be held when calling this method.
    void dispatchHdrLayerInfo(const HdrLayerInfo& info) EXCLUDES(mMutex);

    // Override for IBinder::DeathRecipient
    void binderDied(const wp<IBinder>&) override EXCLUDES(mMutex);

    // Registers an Fps listener that listens to fps updates for the provided layer
    void addListener(const sp<gui::IHdrLayerInfoListener>& listener) EXCLUDES(mMutex);
    // Deregisters an Fps listener
    void removeListener(const sp<gui::IHdrLayerInfoListener>& listener) EXCLUDES(mMutex);

    bool hasListeners() const EXCLUDES(mMutex) {
        std::scoped_lock lock(mMutex);
        return !mListeners.empty();
    }

private:
    mutable std::mutex mMutex;

    struct TrackedListener {
        sp<gui::IHdrLayerInfoListener> listener;
        HdrLayerInfo lastInfo;
    };

    std::unordered_map<wp<IBinder>, TrackedListener, WpHash> mListeners GUARDED_BY(mMutex);
};

} // namespace android