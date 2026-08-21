//
//  DOHideJailbreakAppsController.h
//  Dopamine
//

#import "DOPSListController.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, DOAppConfigListMode) {
    DOAppConfigListModeHide = 0,
    DOAppConfigListModeBlacklist = 1,
};

@interface DOHideJailbreakAppsController : DOPSListController
@property (nonatomic, assign) DOAppConfigListMode mode;
@end

@interface DOBlacklistAppsController : DOHideJailbreakAppsController
@end

NS_ASSUME_NONNULL_END
