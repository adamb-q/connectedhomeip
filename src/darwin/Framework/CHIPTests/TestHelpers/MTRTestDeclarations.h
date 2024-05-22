//
/**
 *    Copyright (c) 2023 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#import <Foundation/Foundation.h>
#import <Matter/Matter.h>

NS_ASSUME_NONNULL_BEGIN

#pragma mark - Declarations for internal methods

@class MTRDeviceClusterData;
// MTRDeviceControllerDataStore.h includes C++ header, and so we need to declare the methods separately
@protocol MTRDeviceControllerDataStoreAttributeStoreMethods
- (nullable NSDictionary<MTRClusterPath *, MTRDeviceClusterData *> *)getStoredClusterDataForNodeID:(NSNumber *)nodeID;
- (void)storeClusterData:(NSDictionary<MTRClusterPath *, MTRDeviceClusterData *> *)clusterData forNodeID:(NSNumber *)nodeID;
- (void)clearStoredClusterDataForNodeID:(NSNumber *)nodeID;
- (void)clearAllStoredClusterData;
- (void)unitTestPruneEmptyStoredClusterDataBranches;
- (NSString *)_endpointIndexKeyForNodeID:(NSNumber *)nodeID;
- (NSString *)_clusterIndexKeyForNodeID:(NSNumber *)nodeID endpointID:(NSNumber *)endpointID;
- (NSString *)_clusterDataKeyForNodeID:(NSNumber *)nodeID endpointID:(NSNumber *)endpointID clusterID:(NSNumber *)clusterID;
@end

// Declare internal methods for testing
@interface MTRDeviceController (Test)
+ (void)forceLocalhostAdvertisingOnly;
- (void)removeDevice:(MTRDevice *)device;
@property (nonatomic, readonly, nullable) id<MTRDeviceControllerDataStoreAttributeStoreMethods> controllerDataStore;
@end

@interface MTRDevice (Test)
- (BOOL)_attributeDataValue:(NSDictionary *)one isEqualToDataValue:(NSDictionary *)theOther;
@end

#pragma mark - Declarations for items compiled only for DEBUG configuration

#ifdef DEBUG
@interface MTRDeviceController (TestDebug)
- (NSDictionary<NSNumber *, NSNumber *> *)unitTestGetDeviceAttributeCounts;
@end

@interface MTRBaseDevice (TestDebug)
// Test function for whitebox testing
+ (id)CHIPEncodeAndDecodeNSObject:(id)object;
@end

@interface MTRDevice (TestDebug)
typedef NS_ENUM(NSUInteger, MTRInternalDeviceState) {
    // Unsubscribed means we do not have a subscription and are not trying to set one up.
    MTRInternalDeviceStateUnsubscribed = 0,
    // Subscribing means we are actively trying to establish our initial subscription (e.g. doing
    // DNS-SD discovery, trying to establish CASE to the peer, getting priming reports, etc).
    MTRInternalDeviceStateSubscribing = 1,
    // InitialSubscriptionEstablished means we have at some point finished setting up a
    // subscription.  That subscription may have dropped since then, but if so it's the ReadClient's
    // responsibility to re-establish it.
    MTRInternalDeviceStateInitialSubscriptionEstablished = 2,
    // Resubscribing means we had established a subscription, but then
    // detected a subscription drop due to not receiving a report on time. This
    // covers all the actions that happen when re-subscribing (discovery, CASE,
    // getting priming reports, etc).
    MTRInternalDeviceStateResubscribing = 3,
    // LaterSubscriptionEstablished meant that we had a subscription drop and
    // then re-created a subscription.
    MTRInternalDeviceStateLaterSubscriptionEstablished = 4,
};

- (void)unitTestInjectEventReport:(NSArray<NSDictionary<NSString *, id> *> *)eventReport;
- (void)unitTestInjectAttributeReport:(NSArray<NSDictionary<NSString *, id> *> *)attributeReport fromSubscription:(BOOL)isFromSubscription;
- (NSUInteger)unitTestAttributesReportedSinceLastCheck;
- (void)unitTestClearClusterData;
- (MTRInternalDeviceState)_getInternalState;
@end
#endif

NS_ASSUME_NONNULL_END
