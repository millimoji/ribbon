//
//  imecore.h
//  iOSKeyboardTemplate
//
//  Created by Shinji Morimitsu on 5/6/18.
//

#ifndef imecore_h
#define imecore_h
#import <Foundation/Foundation.h>

@interface ImeCore : NSObject
- (void) JsonCommand: (NSString*)json;
- (NSString*) GetCompositionText;
- (NSString*) GetDeterminedText;
- (NSString*) GetCandidateJson;
- (Boolean) DoesQuieKeyboard;
@end

#endif /* imecore_h */
