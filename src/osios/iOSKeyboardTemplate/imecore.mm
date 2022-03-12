//
//  imecore.m
//  iOSKeyboardTemplate
//
//  Created by Shinji Morimitsu on 5/6/18.
//

#import <Foundation/Foundation.h>
#import "pch.h"
#import "imecore.h"
#import "inputmodel/InputModel.h"
#import "osios.h"

@implementation ImeCore {
    std::shared_ptr<Ribbon::IJsonInputModel> m_inputModel;
}

- (id)init {
    if (self = [super init]) {
        NSURL *rsrcUrl = [[NSBundle mainBundle] URLForResource:@"Ribbon-ja" withExtension:@"dic"];
        std::string rsrcString = [[rsrcUrl resourceSpecifier] UTF8String];
        size_t lastSlash = rsrcString.find_last_of('/');
        Ribbon::iOS::s_resourceRoot = rsrcString.substr(0, lastSlash + 1);

        m_inputModel = FACTORYCREATENS(Ribbon, JsonInputModel);
    }
    return self;
}

- (void) JsonCommand: (NSString*)json {
    m_inputModel->JsonCommand([json UTF8String]);
}

- (NSString*) GetCompositionText {
    auto compositionText = m_inputModel->GetRawInputModel()->CompositionText();
    if (compositionText) {
        auto utf8display = compositionText->Display().u8str();
        NSString* retVal = [[NSString alloc] initWithUTF8String:utf8display.c_str()];
        return retVal;
    }
    return [[NSString alloc] init];
}

- (NSString*) GetDeterminedText{
    auto utf8text = m_inputModel->GetRawInputModel()->InsertingText().u8str();
    return [[NSString alloc] initWithUTF8String:utf8text.c_str()];
}

- (NSString*) GetCandidateJson {
    return [[NSString alloc] initWithUTF8String: m_inputModel->CandidateState().c_str()];
}

- (Boolean) DoesQuieKeyboard {
    return m_inputModel->GetRawInputModel()->KeyboardState() == Ribbon::ToKeyboard::Quit;
}

@end

