//
//  KeyboardViewController.m
//  iOSKeyboardTemplate
//
//  Copyright (c) 2014 BJH Studios. All rights reserved.
//  questions or comments contact jeff@bjhstudios.com

#import <WebKit/WebKit.h>
#import "KeyboardViewController.h"
#import "imecore.h"

@interface KeyboardViewController () /*<WKUIDelegate, WKNavigationDelegate, WKScriptMessageHandler>*/ <UIWebViewDelegate> {
    ImeCore* _imeCore;
}

//@property (strong, nonatomic) IBOutlet WKWebView *webView;
@property (strong, nonatomic) IBOutlet UIWebView *webView;

@end

@implementation KeyboardViewController

- (id)init {
    if (self = [super init]) {
        _imeCore = [[ImeCore alloc] init];
    }
    return self;
}

- (void)updateViewConstraints {
    [super updateViewConstraints];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    [self initializeKeyboard];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

#pragma mark - TextInput methods

- (void)textWillChange:(id<UITextInput>)textInput {
}

- (void)textDidChange:(id<UITextInput>)textInput {
}

#pragma mark - initialization method

- (void) initializeKeyboard {
    self.webView.delegate = self;
    [[NSURLCache sharedURLCache] setMemoryCapacity:0];
    [[NSURLCache sharedURLCache] removeAllCachedResponses];
    
    NSURL *url = [[NSBundle mainBundle] URLForResource:@"index" withExtension:@"html"];
    NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:url cachePolicy: NSURLRequestReloadIgnoringCacheData timeoutInterval:100000];
    [request setCachePolicy:NSURLRequestReloadIgnoringLocalCacheData];
    
    [self.webView loadRequest:request];
}

-(BOOL)webView:(WKWebView *)webView shouldStartLoadWithRequest:(nonnull NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
    if ([request.URL.scheme isEqualToString:@"mediator"]) {
        if (!_imeCore) {
            _imeCore = [[ImeCore alloc] init];
        }
        NSString* urlEncodedText = [request.URL.resourceSpecifier substringFromIndex:2];
        NSString* jsonText = [urlEncodedText stringByRemovingPercentEncoding];

        [_imeCore JsonCommand:jsonText];
        
        NSString* candidateJs = [NSString stringWithFormat:@"window.MediatorUpdateCandidate('%@')", [_imeCore GetCandidateJson]];
        
        [_webView stringByEvaluatingJavaScriptFromString:candidateJs];
        
        NSString* determined = [_imeCore GetDeterminedText];
        if (determined.length > 0) {
            [self.textDocumentProxy insertText:determined];
        }
        
        if ([_imeCore DoesQuieKeyboard]) {
            [self advanceToNextInputMode];
        }
        return NO;
    }
    return YES;
}

/*
- (void)userContentController:(WKUserContentController *)userContentController didReceiveScriptMessage:(WKScriptMessage *)message
{
    id body = message.body;
    NSString *keyPath = message.name;
    
    if ([body isKindOfClass:[NSString class]]) {
        if ([keyPath isEqualToString:@"callbackHandler"]) {
            NSLog(@"body: %@", body);
            [self.textDocumentProxy insertText:body];
            
        }
    }
}
 */

#pragma mark - key methods

- (IBAction) globeKeyPressed:(id)sender {
    //required functionality, switches to user's next keyboard
    [self advanceToNextInputMode];
}

- (IBAction) keyPressed:(UIButton*)sender {
}

-(IBAction) backspaceKeyPressed: (UIButton*) sender {
    [self.textDocumentProxy deleteBackward];
}

-(IBAction) spaceKeyPressed: (UIButton*) sender {
}

-(void) spaceKeyDoubleTapped: (UIButton*) sender {
 }

-(IBAction) returnKeyPressed: (UIButton*) sender {
    [self.textDocumentProxy insertText:@"\n"];
}

-(IBAction) shiftKeyPressed: (UIButton*) sender {
    [self shiftKeys];
}

-(void) shiftKeyDoubleTapped: (UIButton*) sender {
}

- (void) shiftKeys {
}

- (IBAction) switchKeyboardMode:(UIButton*)sender {
}

@end
