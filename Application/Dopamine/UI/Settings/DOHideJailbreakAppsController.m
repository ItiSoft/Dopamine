//
//  DOHideJailbreakAppsController.m
//  Dopamine
//

#import "DOHideJailbreakAppsController.h"
#import "DOEnvironmentManager.h"
#import "DOUIManager.h"
#import <CoreServices/LSApplicationProxy.h>

@interface LSApplicationProxy (HideJailbreakApps)
@property (nonatomic, readonly) NSString *applicationIdentifier;
@property (nonatomic, readonly) NSString *localizedName;
@property (nonatomic, readonly) NSString *applicationType;
@end

@interface LSApplicationWorkspace : NSObject
+ (instancetype)defaultWorkspace;
- (NSArray <LSApplicationProxy *> *)allApplications;
@end

@interface DOHideJailbreakAppsController ()
{
    NSArray <LSApplicationProxy *> *_userApps;
}
@end

@implementation DOHideJailbreakAppsController

- (instancetype)init
{
    self = [super init];
    if (self) {
        _mode = DOAppConfigListModeHide;
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    if (self.mode == DOAppConfigListModeBlacklist) {
        self.title = DOLocalizedString(@"Settings_Blacklist_Apps");
    }
    else {
        self.title = DOLocalizedString(@"Settings_Hide_Jailbreak_Per_App");
    }
    [self loadUserApps];
}

- (void)loadUserApps
{
    Class workspaceClass = NSClassFromString(@"LSApplicationWorkspace");
    if (!workspaceClass) {
        _userApps = @[];
        return;
    }

    LSApplicationWorkspace *workspace = [workspaceClass defaultWorkspace];
    NSMutableArray <LSApplicationProxy *> *userApps = [NSMutableArray new];
    NSString *dopamineId = [NSBundle mainBundle].bundleIdentifier;

    for (LSApplicationProxy *app in [workspace allApplications]) {
        if (!app.isInstalled) continue;
        if (![app.applicationType isEqualToString:@"User"]) continue;

        NSString *bundleId = app.applicationIdentifier;
        if (bundleId.length == 0) continue;
        if (dopamineId && [bundleId isEqualToString:dopamineId]) continue;

        [userApps addObject:app];
    }

    [userApps sortUsingComparator:^NSComparisonResult(LSApplicationProxy *a, LSApplicationProxy *b) {
        NSString *nameA = a.localizedName ?: a.applicationIdentifier;
        NSString *nameB = b.localizedName ?: b.applicationIdentifier;
        return [nameA localizedCaseInsensitiveCompare:nameB];
    }];

    _userApps = userApps;
}

- (id)specifiers
{
    if (_specifiers) return _specifiers;

    NSMutableArray *specifiers = [NSMutableArray new];

    PSSpecifier *group = [PSSpecifier emptyGroupSpecifier];
    if (self.mode == DOAppConfigListModeBlacklist) {
        [group setProperty:DOLocalizedString(@"Hint_Blacklist_Apps") forKey:@"footerText"];
    }
    else {
        [group setProperty:DOLocalizedString(@"Hint_Hide_Jailbreak_Per_App") forKey:@"footerText"];
    }
    [specifiers addObject:group];

    for (LSApplicationProxy *app in _userApps) {
        NSString *title = app.localizedName.length ? app.localizedName : app.applicationIdentifier;
        PSSpecifier *specifier = [PSSpecifier preferenceSpecifierNamed:title
                                                                target:self
                                                                   set:@selector(setAppEnabled:specifier:)
                                                                   get:@selector(readAppEnabled:)
                                                                detail:nil
                                                                  cell:PSSwitchCell
                                                                  edit:nil];
        [specifier setProperty:@YES forKey:@"enabled"];
        [specifier setProperty:app.applicationIdentifier forKey:@"bundleIdentifier"];
        [specifier setProperty:app.applicationIdentifier forKey:@"key"];
        [specifiers addObject:specifier];
    }

    if (_userApps.count == 0) {
        PSSpecifier *empty = [PSSpecifier preferenceSpecifierNamed:DOLocalizedString(@"Hide_Jailbreak_No_Apps")
                                                            target:self
                                                               set:nil
                                                               get:nil
                                                            detail:nil
                                                              cell:PSStaticTextCell
                                                              edit:nil];
        [empty setProperty:@NO forKey:@"enabled"];
        [specifiers addObject:empty];
    }

    _specifiers = specifiers;
    return _specifiers;
}

- (id)readAppEnabled:(PSSpecifier *)specifier
{
    NSString *bundleId = [specifier propertyForKey:@"bundleIdentifier"];
    DOEnvironmentManager *env = [DOEnvironmentManager sharedManager];
    if (self.mode == DOAppConfigListModeBlacklist) {
        return @([env isAppBlacklisted:bundleId]);
    }
    return @([env isJailbreakHiddenForApp:bundleId]);
}

- (void)setAppEnabled:(id)value specifier:(PSSpecifier *)specifier
{
    NSString *bundleId = [specifier propertyForKey:@"bundleIdentifier"];
    DOEnvironmentManager *env = [DOEnvironmentManager sharedManager];
    if (self.mode == DOAppConfigListModeBlacklist) {
        [env setAppBlacklisted:[value boolValue] forBundleIdentifier:bundleId];
    }
    else {
        [env setJailbreakHidden:[value boolValue] forApp:bundleId];
    }
}

@end

@implementation DOBlacklistAppsController

- (DOAppConfigListMode)mode
{
    return DOAppConfigListModeBlacklist;
}

@end
