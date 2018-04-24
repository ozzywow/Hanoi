//
//  PlayScene.m
//  Hanoi
//
//  Created by Jinguk Hong on 11. 2. 26..
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#import "PlayScene.h"
#import "Discus.h"
#import "PlaySceneTouchHandlerLayer.h"
#import "SoundFactory.h"
#import "RankScene.h"
#import "MainScene.h"
#import "UserDataManager.h"
#import "MUIAlertView.h"
#import "BankScene.h"



void PrintStyle( CCNode* parent, NSString* str, int fontSize, CGPoint pos, int anType )
{
	
	CGPoint anchorP = CGPointZero ;
	if( anType == LEFT )
		anchorP = ccp(0,0) ;
	else if( anType == RIGHT )
		anchorP = ccp(1, 0) ;
	else if( anType == CENTER )
		anchorP = ccp(0.5, 0) ;
	
	
	CCLabelTTF* label0 = [CCLabelTTF labelWithString:str fontName:@"Arial" fontSize:fontSize] ;
	label0.anchorPoint = anchorP ;
	label0.position = ccp(pos.x-1, pos.y) ;
	label0.color = ccc3(0,0,0) ;
	[parent addChild:label0 z:TEXT_Z_DEF] ;
	
	CCLabelTTF* label1 = [CCLabelTTF labelWithString:str fontName:@"Arial" fontSize:fontSize] ;
	label1.anchorPoint = anchorP ;
	label1.position = ccp(pos.x+1, pos.y) ;
	label1.color = ccc3(0,0,0) ;
	[parent addChild:label1 z:TEXT_Z_DEF] ;
	
	CCLabelTTF* label2 = [CCLabelTTF labelWithString:str fontName:@"Arial" fontSize:fontSize] ;
	label2.anchorPoint = anchorP ;
	label2.position = ccp(pos.x, pos.y-1) ;
	label2.color = ccc3(0,0,0) ;
	[parent addChild:label2 z:TEXT_Z_DEF] ;
	
	CCLabelTTF* label3 = [CCLabelTTF labelWithString:str fontName:@"Arial" fontSize:fontSize] ;
	label3.anchorPoint = anchorP ;
	label3.position = ccp(pos.x, pos.y+1) ;
	label3.color = ccc3(0,0,0) ;
	[parent addChild:label3 z:TEXT_Z_DEF] ;
	
	CCLabelTTF* label = [CCLabelTTF labelWithString:str fontName:@"Arial" fontSize:fontSize] ;
	label.anchorPoint = anchorP ;
	label.position = pos ;
	label.color = ccc3(255,255,255) ;
	[parent addChild:label z:TEXT_Z_DEF] ;
}



static CGPoint arrPosOfPole[3] ;


@implementation PlayScene




// on "init" you need to initialize your instance
-(id) initWithDiscusNum:(int)numOfDiscus  mode:(bool)bRainking 
{
	// always call "super" init
	// Apple recommends to re-assign "self" with the "super" return value
	if( (self=[super init] )) {
		
		
		m_isRakingMode = bRainking;
		
		m_countOfDiscus = numOfDiscus ;
		
		arrPosOfPole[0]  = ccp(105,140);
		arrPosOfPole[1]  = ccp(241,140);
		arrPosOfPole[2]  = ccp(377,140);
			
		NSMutableArray*	pPoleQueue0 = [NSMutableArray array] ;
		NSMutableArray*	pPoleQueue1 = [NSMutableArray array] ;
		NSMutableArray*	pPoleQueue2 = [NSMutableArray array] ;
		m_pPole = [[NSArray alloc] initWithObjects:pPoleQueue0, pPoleQueue1, pPoleQueue2, nil] ;
		m_arrDiscurs = [[NSMutableArray alloc] init] ;
		
	
		
		// 터치 이벤트를 담당할 Layer를 만든 후 GameScene에 넣습니다.
		m_touchHanderLayer = [[PlaySceneTouchHandlerLayer node] initWithScene:self] ;
		[self addChild:m_touchHanderLayer z:tagTouchingLayer tag:tagTouchingLayer] ;
		
		
		CCSprite* backGround = [CCSprite spriteWithFile:@"main_bg.png"] ;
		if( backGround )
		{
			backGround.anchorPoint = CGPointZero;
			//backGround.size.width = size.width ;
			[self addChild:backGround z:tagBG tag:tagBG] ;
		}
		
	
		CCSprite* pCoinImg = [CCSprite spriteWithFile:@"gold_coin.png"];
		pCoinImg.anchorPoint = ccp(0.5, 0.5);
		pCoinImg.position =  ccp( 400, 290 );
		//[self addChild:pCoinImg z:tagInfoText tag:tagInfoText] ;
		
		m_coinLayer = [CCLayer node];
		[m_coinLayer addChild:pCoinImg z:tagCoin tag:tagCoin];
		[self addChild:m_coinLayer z:tagInfoText tag:tagInfoText];
		
		
		m_selectionMark = [CCSprite spriteWithFile:@"block_selectEff.png"] ;
		if( m_selectionMark )
		{
			[m_selectionMark setBlendFunc:(ccBlendFunc){GL_SRC_ALPHA, GL_ONE}];
			[m_selectionMark setOpacity:100];
			m_selectionMark.anchorPoint = ccp(0.5,0) ;
			[self addChild:m_selectionMark z:tagSelectionMark tag:tagSelectionMark] ;
			m_selectionMark.position = ccp(500,500) ;
		}
		
		m_deselectionMark = [CCSprite spriteWithFile:@"block_selectEff.png"] ;
		if( m_deselectionMark )
		{
			[m_deselectionMark setBlendFunc:(ccBlendFunc){GL_SRC_ALPHA, GL_ONE}];
			[m_deselectionMark setOpacity:100];
			m_deselectionMark.anchorPoint = ccp(0.5,0) ;
			[self addChild:m_deselectionMark z:tagSelectionMark tag:tagSelectionMark] ;
			m_deselectionMark.position = ccp(500,500) ;
		}
		
		
		
		m_labelTime = [CCLabelTTF labelWithString:@" 0: 0: 0" fontName:@"Arial" fontSize:20] ;
		m_labelTime.color = ccc3(255,255,255) ;
		m_labelTime.position =  ccp( 415, 310 );
		[self addChild:m_labelTime z:tagInfoText tag:tagInfoText];
		
		
		m_discusLayer = [CCLayer node];
		[self addChild:m_discusLayer];
		
		
		int offset = 10 / m_countOfDiscus ;
		for (int i=0; i<m_countOfDiscus; ++i) 
		{
			Discus* pDiscus = [[Discus node] initWithDiscusID:i*offset] ;
			if( pDiscus )
			{
				[m_discusLayer addChild:pDiscus z:tagDiscus tag:tagDiscus] ;
				[m_arrDiscurs addObject:pDiscus] ;
			}
		}

		BOOL soundOpt = [[UserDataManager sharedUserDataManager] GetSoundOpt];
		[self DrawMenu:soundOpt];
		[self InitGame];
		[self ResetGame] ;		
		[self DrawDiscus] ;
		[self DrawInfoText] ;
		
	
		if( false == m_isRakingMode && m_countOfDiscus == 3 )
		{
			if( m_imgCount )
			{
				[m_imgCount removeFromParentAndCleanup:NO];
			}
			m_imgCount = [CCSprite spriteWithFile:@"arrow.png"];
			[self addChild:m_imgCount];
			m_imgCount.anchorPoint = ccp(0.5, 0.5);
			m_imgCount.position = ccp(480/2, 220);
			
			CCSprite* text = nil;
			NSString *locale = [[NSLocale preferredLanguages] objectAtIndex:0];
			if ([locale isEqualToString:@"ko"]) 
			{
				text = [CCSprite spriteWithFile:@"text_kr.png"];
			}
			else 
			{
				text = [CCSprite spriteWithFile:@"text_en.png"];
			}
			text.anchorPoint = ccp(0.5, 0.5);
			text.position = ccp(m_imgCount.contentSize.width/2, (m_imgCount.contentSize.height/2)-80);
			[m_imgCount addChild:text];
			
		}
		
		
		
		
		/*
		if( NO == [[UserDataManager sharedUserDataManager] GetCart])
		{
			if( m_countOfDiscus > MAX_FREE_LEVEL )
			{
				
				
				CCMenuItemImage* pLockMenu = [CCMenuItemImage itemFromNormalImage:@"lock_icon.png" 
																	selectedImage:@"lock_icon_s.png" 
																		   target:self 
																		 selector:@selector(callbackLockBtn:)];	
				
				CCMenu* pMenu = [CCMenu menuWithItems:pLockMenu, nil];
				
				pMenu.position = ccp(105, 200);
				[self addChild:pMenu];
				
				
			}
		}*/

		
#ifdef AD_VER		
		if(  NO == [[UserDataManager sharedUserDataManager] GetCart] )
		{
			//[[UserDataManager sharedUserDataManager] viewAd:true];
		}
		else 
		{
			//[[UserDataManager sharedUserDataManager] viewAd:false];
		}

#endif

		

	}
	return self;
}

// on "dealloc" you need to release all your retained objects
- (void) dealloc
{
	// in case you have something to dealloc, do it in this method
	// in this particular example nothing needs to be released.
	// cocos2d will automatically release all the children (Label)
	
	// don't forget to call "super dealloc"
	
	[[SoundFactory sharedSoundFactory] StopSound:@"BGM2"] ;
	
	[m_pPole release] ;
	[m_arrDiscurs release] ;

	
	
	[super dealloc];
}


// 상금정보를 요청한다.
-(void) requestPrize:(int)level
{
	
	[[UserDataManager sharedUserDataManager] ToggleIndicator:YES];
	
	NSLog(@"Requesting scores...");
	
	CLScoreServerRequest *request = [[CLScoreServerRequest alloc] initWithGameName:@"TowerOfHanoi" delegate:self];
	
	NSString *cat = [NSString stringWithFormat:@"prize_%d", level] ;
	
	// request All time Scores: the only supported version as of v0.2
	// request best 15 scores (limit:15, offset:0)
	[request requestScores:kQueryWeek limit:1 offset:0 flags:kQueryFlagIgnore category:cat];
	
	// Release. It won't be freed from memory until the connection fails or suceeds
	[request release];
	
	
}


-(void) DrawPrize:(NSArray*)prizes
{
	
	NSMutableArray *mutable = [NSMutableArray arrayWithArray:prizes];
	int cnt = mutable.count;
	if(cnt == 0 ){ return ;}
	
	
	
	NSDictionary	*s = [mutable objectAtIndex:0];
	NSInteger gold = [[s objectForKey:@"cc_score"] integerValue];
	
	m_prize = gold;
}


#pragma mark -
#pragma mark ScoreRequest Delegate

-(void) scoreRequestOk: (id) sender
{	
	NSLog(@"scoreRequestOk");
	NSArray *scores = [sender parseScores];	
	
	[self DrawPrize:scores];
	
	[[UserDataManager sharedUserDataManager] ToggleIndicator:NO];
}

-(void) scoreRequestFail: (id) sender
{
	NSLog(@"scoreRequestFail");
	m_prize = 0;
	
	[[UserDataManager sharedUserDataManager] ToggleIndicator:NO];
}



-(void) DrawMenu:(BOOL)SoundOpt
{
	[self removeChildByTag:1234 cleanup:NO];
	CCMenuItemImage* homeMenuItem = [CCMenuItemImage	itemFromNormalImage:@"btn_home_n.png" 
														   selectedImage:@"btn_home_s.png" 
																  target:self 
																selector:@selector(callbackOnPushed_homeMenuItem:)];
	
	
	CCMenuItemImage* resetMenuItem = [CCMenuItemImage	itemFromNormalImage:@"btn_refresh_n.png" 
															selectedImage:@"btn_refresh_s.png" 
																   target:self 
																 selector:@selector(callbackOnPushed_resetMenuItem:)];
	
	CCMenuItemImage* prevMenuItem = [CCMenuItemImage	itemFromNormalImage:@"btn_prev.png" 
														   selectedImage:@"btn_prev.png" 
																  target:self 
																selector:@selector(callbackOnPushed_prevMenuItem:)];
	
	CCMenuItemImage* nextMenuItem = [CCMenuItemImage	itemFromNormalImage:@"btn_next.png" 
														   selectedImage:@"btn_next.png" 
																  target:self 
																selector:@selector(callbackOnPushed_nextMenuItem:)];
	
	NSString* iconName = nil;
	if( NO == SoundOpt )
	{
		iconName = [NSString stringWithFormat:@"btn_speaker_on.png"];
	}
	else {
		iconName = [NSString stringWithFormat:@"btn_speaker_off.png"];
	}
	
	CCMenuItemImage* speakerMenuItem = [CCMenuItemImage	itemFromNormalImage:iconName 
															  selectedImage:iconName 
																	 target:self 
																   selector:@selector(callbackOnPushed_speakerMenuItem:)];
	
	
	CCMenu* mainMenuTop = [CCMenu menuWithItems:homeMenuItem, resetMenuItem, nextMenuItem, prevMenuItem, speakerMenuItem, nil] ;
	[mainMenuTop alignItemsVerticallyWithPadding:5] ;
	mainMenuTop.position = ccp(20,220) ; 
	[self addChild:mainMenuTop z:1234 tag:1234] ;
	
	
}

-(void) InitGame
{
	[self removeChildByTag:tagPopup cleanup:NO];
	m_isIng = NONE ;
	
	
	int minCoin = (pow((m_countOfDiscus-3+1),2))*10;
	int currCoin = [[UserDataManager sharedUserDataManager] GetCoin];
	if (m_isRakingMode) 
	{
		// 코인이 부족해서 랭킹모드를 시작할 수 없다는 표시를 출력해줄것
		
		if (currCoin < minCoin) 
		{
			CCSprite* textBG =  [CCSprite spriteWithFile:@"text_empty.png"];
			textBG.anchorPoint = ccp(0.5, 0.5);
			textBG.position = ccp(480/2, 320/2);
			[self addChild:textBG];
			
			
			
			NSString* pCommentText = nil;
			NSString *locale = [[NSLocale preferredLanguages] objectAtIndex:0];
			if ([locale isEqualToString:@"ko"]) 
			{
				pCommentText = [NSString stringWithFormat:@"  %d레벨 [Battle] game 에서는 최소 금화 %d 이상이 필요합니다.\n  상점 또는 [Training] 게임을 통해 금화를 모을 수 있습니다."
								,m_countOfDiscus , minCoin] ;
			}
			else 
			{
				pCommentText = [NSString stringWithFormat:@"Sorry! \n This is the [Battle] game. You can play %d level when you have %d Gold more. You can get gold coins at the store or the [Training] game."
								,m_countOfDiscus, minCoin] ;
			}
			
			// lable of shadow
			CGSize sizeOfTextBox;
			sizeOfTextBox.width = 320;
			sizeOfTextBox.height = 75 ;
			
			pCommentText = [pCommentText stringByReplacingOccurrencesOfString:@"/n" withString:@"\n"];
		
			CCLabelTTF* pCommentLabel = [CCLabelTTF labelWithString:pCommentText dimensions:sizeOfTextBox alignment:CCTextAlignmentLeft fontName:@"Arial" fontSize:15] ;
			pCommentLabel.anchorPoint = ccp(0.5, 0.5);
			pCommentLabel.position = ccp(textBG.contentSize.width/2, (textBG.contentSize.height/2)+45);
			[textBG addChild:pCommentLabel] ;
		
			
			
			CCMenuItemImage* pNeedCoinMenu = [CCMenuItemImage itemFromNormalImage:@"btn_cart.png" 
																	selectedImage:@"btn_cart_s.png" 
																		   target:self 
																		 selector:@selector(callbackNeedCoinBtn:)];	
			
			CCMenu* pMenu = [CCMenu menuWithItems:pNeedCoinMenu, nil];
			
			pMenu.position = ccp(350, 140);
			[m_coinLayer addChild:pMenu];
			
			id action = [CCBlink actionWithDuration:100000 blinks:100000];
			[m_coinLayer runAction:action];
			
		}
	}
	
	
	m_countDown=3;
	[m_labelTime setString:@" 0: 0: 0"];
	
	
	
	
	
	//int currCoin = [[UserDataManager sharedUserDataManager] GetCoin];
	NSString* pCoin = [NSString stringWithFormat:@"x%d", currCoin] ;
	if( nil == m_pCoinLabel )
	{
		m_pCoinLabel = [CCLabelTTF labelWithString:pCoin fontName:@"Arial" fontSize:20];
		m_pCoinLabel.color = ccc3(255,255,255) ;
		m_pCoinLabel.anchorPoint = ccp(0, 0.5);
		m_pCoinLabel.position =  ccp( 410, 290 );
		//[self addChild:m_pCoinLabel z:tagInfoText tag:tagInfoText];
		
		[m_coinLayer addChild:m_pCoinLabel z:tagInfoText tag:tagInfoText];
	
		NSString* pMinCoin=nil;
		if (m_isRakingMode) 
		{
			pMinCoin = [NSString stringWithFormat:@"-%d", minCoin];
			
			CCLabelTTF* pMinCoinLabel = [CCLabelTTF labelWithString:pMinCoin fontName:@"Arial" fontSize:20] ;
			pMinCoinLabel.color = ccc3(0,0,255);
			pMinCoinLabel.anchorPoint = ccp(0, 0.5);
			pMinCoinLabel.position =  ccp( 410, 270 );
			[m_coinLayer addChild:pMinCoinLabel z:tagInfoText tag:tagInfoText];
			
			
			
		}
		else 
		{
			int awardCoin = pow((m_countOfDiscus-3+1),2);
			pMinCoin = [NSString stringWithFormat:@"+%d", awardCoin];
			
			CCLabelTTF* pMinCoinLabel = [CCLabelTTF labelWithString:pMinCoin fontName:@"Arial" fontSize:20] ;
			pMinCoinLabel.color = ccc3(255,0,0);
			pMinCoinLabel.anchorPoint = ccp(0, 0.5);
			pMinCoinLabel.position =  ccp( 410, 270 );
			[m_coinLayer addChild:pMinCoinLabel z:tagInfoText tag:tagInfoText];
			
		}
		
		
	}
	else 
	{
		[m_pCoinLabel setString:pCoin];
	}
	

}


-(int) GetPlayState
{
	return m_isIng;
}

-(void) Start
{
	m_isIng = PLAY;
	
	
	[[SoundFactory sharedSoundFactory] PlaydSound:@"FX0070" volume:0.4] ;
	if( YES ==[[UserDataManager sharedUserDataManager] GetSoundOpt] )
	{
		AVAudioPlayer* pAudio = [[SoundFactory sharedSoundFactory] PlaydSound:@"BGM2" volume:0.0];
		if( pAudio )
		{
			pAudio.numberOfLoops = 1000;
		}
	}
	else 
	{
		AVAudioPlayer* pAudio = [[SoundFactory sharedSoundFactory] PlaydSound:@"BGM2"];
		if( pAudio )
		{
			pAudio.numberOfLoops = 1000;
		}
	}
	
	
	[m_dateTime release] ;
	m_dateTime = [[NSDate alloc] init] ;
	
	
	
	id delay = [CCDelayTime actionWithDuration:0.1] ;
	id callFunc_drawTime = [CCCallFunc actionWithTarget:self selector:@selector(DrawTime)] ;

	id actionToDrawTime = [CCSequence actions:callFunc_drawTime, delay, nil];

	
	
	
	m_actionTimeRun = [CCRepeatForever actionWithAction:actionToDrawTime];

	
	[self runAction:m_actionTimeRun] ;
	
	
}


-(void) Finished
{
	m_isIng = COMPLATE;
	[self stopAction:m_actionTimeRun];
	
	NSTimeInterval elapsedTime = 0 - [m_dateTime timeIntervalSinceNow];
	//int minutes = floor(elapsedTime/60);
	//int seconds = trunc(elapsedTime - minutes * 60);
	//int ms = trunc((elapsedTime - seconds)*10);
	
	
	
	
	[[SoundFactory sharedSoundFactory] StopSound:@"BGM2"] ;
	
	
	m_mastTime = elapsedTime*100 ;

	
	int bestRecordTime = [[UserDataManager sharedUserDataManager] GetBestRecord:m_countOfDiscus];
	if( m_mastTime < 100 ) m_mastTime = 100;
	if( bestRecordTime == 0 || (bestRecordTime > 0  && bestRecordTime > m_mastTime) )
	{
		[[UserDataManager sharedUserDataManager] SetBestRecord:m_countOfDiscus time:m_mastTime];
		
		
		id delay = [CCDelayTime actionWithDuration:2.0];
		id callFucn =[CCCallFunc actionWithTarget:self selector:@selector(MessagePopup)] ;
		[self runAction:[CCSequence actions:delay, callFucn, nil] ];
		
		if( m_isRakingMode == false )
		{
			int currCoin = [[UserDataManager sharedUserDataManager] GetCoin];
			currCoin += pow((m_countOfDiscus-3+1),2)*2;
			[[UserDataManager sharedUserDataManager] SetCoin:currCoin];
		}

		
	}
	else 
	{
		id delay = [CCDelayTime actionWithDuration:2.0];
		id callFucn =[CCCallFunc actionWithTarget:self selector:@selector(RankingMessagePopup)] ;
		[self runAction:[CCSequence actions:delay, callFucn, nil] ];	
		
		if( m_isRakingMode == false )
		{
			int currCoin = [[UserDataManager sharedUserDataManager] GetCoin];
			currCoin += pow((m_countOfDiscus-3+1),2);
			[[UserDataManager sharedUserDataManager] SetCoin:currCoin];
		}
	}

	int curLevel = [[UserDataManager sharedUserDataManager] GetLevel] ;
    int nextLevel = m_countOfDiscus + 1;
#ifdef AD_VER
    bool bCart = [[UserDataManager sharedUserDataManager] GetCart];
    if( false == bCart && nextLevel > MAX_LEVEL_FOR_LITE_VERTION )
    {
        nextLevel = m_countOfDiscus;
        
        id delay = [CCDelayTime actionWithDuration:2.0];
		id callFucn =[CCCallFunc actionWithTarget:self selector:@selector(LITE_VERSION_MessagePopup)] ;
		[self runAction:[CCSequence actions:delay, callFucn, nil] ];
        return;
    }
    
#endif //AD_VER
	if ( curLevel < nextLevel)
	{

		[[UserDataManager sharedUserDataManager] SetLevel:nextLevel] ;

	}

		
}

static BOOL g_isNewRecord = NO;
-(void) MessagePopup
{
	
	if (m_isRakingMode) 
	{
		g_isNewRecord = YES;
		//새로운 기록을 갱신했다. 랭킹에 올릴것인가..
		
		NSString *locale = [[NSLocale preferredLanguages] objectAtIndex:0];
		if ([locale isEqualToString:@"ko"]) 
		{
			NSString* recordMsg = [NSString stringWithFormat:@"당신의 신기록을 월드랭킹에 업데이트 하시겠습니까? \n \
								   기록:%g(초), 이동:%d", m_mastTime*0.01, m_moveCount];
			MUIAlertView* alertView = [[[MUIAlertView alloc] 
										initWithTitle:@"기록 달성" 
										message:recordMsg
										numberOfTextFields:0 
										delegate:self 
										cancelButtonTitle:@"취소" 
										otherButtonTitle:@"승인"] autorelease] ;
			[alertView show] ;
			
		}
		else 
		{
			NSString* recordMsg = [NSString stringWithFormat:@"Update your record to the world ranking! \n \
								   Time:%g(sec), Move:%d", m_mastTime*0.01, m_moveCount];
			MUIAlertView* alertView = [[[MUIAlertView alloc] 
										initWithTitle:@"Record breaking!" 
										message:recordMsg
										numberOfTextFields:0 
										delegate:self 
										cancelButtonTitle:@"Cancel" 
										otherButtonTitle:@"OK"] autorelease] ;
			[alertView show] ;
			
		}
	}
	else 
	{
		
		
		[[SoundFactory sharedSoundFactory] PlaydSound:@"drop_coin"] ;
		
		CCSprite* pMSGBG = [CCSprite spriteWithFile:@"text_empty.png"];
		CCSprite* pCoinBank = [CCSprite spriteWithFile:@"coin_bank.png"] ;
		if (pCoinBank) 
		{
			pCoinBank.anchorPoint = ccp(0.5, 0.5);
			pCoinBank.position = ccp( 100, 110);
			
		}
		
		int awardCoin = pow((m_countOfDiscus-3+1),2);
		NSString* recordMsg = [NSString stringWithFormat:@"+  %d x2", awardCoin];
		CCLabelTTF* pPrizeMsg = [CCLabelTTF labelWithString:recordMsg fontName:@"Arial" fontSize:40];
		if (pPrizeMsg) 
		{
			pPrizeMsg.position = ccp( 210, 110);
			
		}
		
		[pMSGBG addChild:pCoinBank];
		[pMSGBG addChild:pPrizeMsg];
		pMSGBG.anchorPoint = ccp(0.5, 0.5);
		pMSGBG.position = ccp( 480/2, 320/2);
		pMSGBG.scale = 0.5;
		
		
		id action = [CCScaleTo actionWithDuration:0.1 scale:1.0];
		[pMSGBG runAction:action];

		[self addChild:pMSGBG z:tagPopup tag:tagPopup];
	}

	

		
}


-(void) RankingMessagePopup
{
	if (m_isRakingMode) 
	{
		g_isNewRecord = NO;
		//새로운 기록을 갱신했다. 랭킹에 올릴것인가..
		
		NSString *locale = [[NSLocale preferredLanguages] objectAtIndex:0];
		if ([locale isEqualToString:@"ko"]) 
		{
			NSString* recordMsg = [NSString stringWithFormat:@"당신의 신기록을 월드랭킹에 업데이트 하시겠습니까? \n \
								   기록:%g(초), 이동:%d", m_mastTime*0.01, m_moveCount];
			MUIAlertView* alertView = [[[MUIAlertView alloc] 
										initWithTitle:@"기록 달성" 
										message:recordMsg
										numberOfTextFields:0 
										delegate:self 
										cancelButtonTitle:@"취소" 
										otherButtonTitle:@"승인"] autorelease] ;
			[alertView show] ;
			
		}
		else 
		{
			NSString* recordMsg = [NSString stringWithFormat:@"Update your record to the world ranking! \n \
								   Time:%g(sec), Move:%d", m_mastTime*0.01, m_moveCount];
			MUIAlertView* alertView = [[[MUIAlertView alloc] 
										initWithTitle:@"Record breaking!" 
										message:recordMsg
										numberOfTextFields:0 
										delegate:self 
										cancelButtonTitle:@"Cancel" 
										otherButtonTitle:@"OK"] autorelease] ;
			[alertView show] ;
			
		}
	}
	else 
	{
		[[SoundFactory sharedSoundFactory] PlaydSound:@"drop_coin"] ;
		
		CCSprite* pMSGBG = [CCSprite spriteWithFile:@"text_empty.png"];
		CCSprite* pCoinBank = [CCSprite spriteWithFile:@"coin_bank.png"] ;
		if (pCoinBank) 
		{
			pCoinBank.anchorPoint = ccp(0.5, 0.5);
			pCoinBank.position = ccp( 100, 110);
			
		}
		
		int awardCoin = pow((m_countOfDiscus-3+1),2);
		NSString* recordMsg = [NSString stringWithFormat:@"x  %d", awardCoin];
		CCLabelTTF* pPrizeMsg = [CCLabelTTF labelWithString:recordMsg fontName:@"Arial" fontSize:40];
		if (pPrizeMsg) 
		{
			pPrizeMsg.position = ccp( 210, 110);
			
		}
		
		[pMSGBG addChild:pCoinBank];
		[pMSGBG addChild:pPrizeMsg];
		pMSGBG.anchorPoint = ccp(0.5, 0.5);
		pMSGBG.position = ccp( 480/2, 320/2);
		pMSGBG.scale = 0.5;
		
		
		id action = [CCScaleTo actionWithDuration:0.1 scale:1.0];
		[pMSGBG runAction:action];
		
		[self addChild:pMSGBG z:tagPopup tag:tagPopup];		
	}
	
	
		
}



-(void) LITE_VERSION_MessagePopup
{
	NSString *locale = [[NSLocale preferredLanguages] objectAtIndex:0];
    if ([locale isEqualToString:@"ko"])
    {
        NSString* recordMsg = [NSString stringWithFormat:@"올림픽 버젼은 5레벨 이상 플레이할 수 없습니다. \n \
                               프로버젼을 이용해주세요."];
        MUIAlertView* alertView = [[[MUIAlertView alloc]
                                    initWithTitle:@"LITE VERSION"
                                    message:recordMsg
                                    numberOfTextFields:0
                                    delegate:self
                                    cancelButtonTitle:@"취소"
                                    otherButtonTitle:@"승인"] autorelease] ;
        [alertView show] ;
        
    }
    else
    {
        NSString* recordMsg = [NSString stringWithFormat:@"Sorry!! Can not play level %d over this LITE Version! \n Play with the PRO Version please!", MAX_LEVEL_FOR_LITE_VERTION];
        MUIAlertView* alertView = [[[MUIAlertView alloc]
                                    initWithTitle:@"LITE VERSION"
                                    message:recordMsg
                                    numberOfTextFields:0
                                    delegate:self
                                    cancelButtonTitle:@"Cancel"
                                    otherButtonTitle:@"OK"] autorelease] ;
        [alertView show] ;
        
    }

	
    
}



-(void) alertView:(UIAlertView*)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
	
	if(  m_isIng == NONE && NO == [[UserDataManager sharedUserDataManager] GetCart])
	{
		
	
		if (buttonIndex == 0 ) 
		{
			
		}
		else 
		{
			NSString *locale = [[NSLocale preferredLanguages] objectAtIndex:0];
			if ([locale isEqualToString:@"ko"]) 
			{
				//한쿡
				//NSURL *appStoreUrl = [NSURL URLWithString:@"http://itunes.apple.com/app/id432096485?mt=8"];
                NSURL *appStoreUrl = [NSURL URLWithString:@"http://itunes.apple.com/app/id426632681?mt=8"];
				[[UIApplication sharedApplication] openURL:appStoreUrl];
			}
			else 
			{
				//미쿡
				NSURL *appStoreUrl = [NSURL URLWithString:@"http://itunes.apple.com/app/id426632681?mt=8"];
				[[UIApplication sharedApplication] openURL:appStoreUrl];
			}
		}
		
	}
	else 
	{
		if (buttonIndex == 0 ) 
		{
			
			[self InitGame];
			[self ResetGame] ;		
			[self DrawDiscus] ;
			[self DrawInfoText] ;
			
		}
		else 
		{
			NSLog(@"확인") ;
			
#ifdef AD_VER		
			//[[UserDataManager sharedUserDataManager] viewAd:false];
#endif
			[self GoRankingPage] ;		
		}
		
	}

	
}




-(void) GoRankingPage
{
	RankScene* rankScene = [[RankScene node] initWithLevel:m_countOfDiscus moveCount:m_moveCount spentTime:m_mastTime] ;
	[[CCDirector sharedDirector] replaceScene:[CCTransitionFade transitionWithDuration:0.2 scene:rankScene] ] ;	//슬라이드

}

-(void) DrawTime
{
	NSTimeInterval elapsedTime = 0 - [m_dateTime timeIntervalSinceNow];
	int minutes = floor(elapsedTime/60);
	int seconds = trunc(elapsedTime - minutes * 60);
	int ms = trunc((elapsedTime-(minutes*60)- seconds)*10);
	
	NSString* pStrTime = [NSString stringWithFormat:@"%2d:%2d:%2d", minutes, seconds, ms] ;
	[m_labelTime setString:pStrTime] ;
	
	if (elapsedTime < 0) 
	{
		[[SoundFactory sharedSoundFactory] PlaydSound:@"FX0066" volume:0.3] ;
		
		MainScene* mainScene = [MainScene node] ;
		[[CCDirector sharedDirector] replaceScene:[CCTransitionFade transitionWithDuration:0.2 scene:mainScene] ] ;	//슬라이드
		
#ifdef AD_VER
		//[[UserDataManager sharedUserDataManager] viewAd:false];
#endif
	}

}



-(void) ResetGame 
{

	m_moveCount = 0 ;
	[m_touchHanderLayer Reset] ;
	
	
	m_selectionMark.position = ccp(500,500) ;
	m_deselectionMark.position = ccp(500,500) ;
	
	for (int i=0; i < 3; ++i) 
	{
		NSMutableArray* pPoleQueue = [m_pPole objectAtIndex:i] ;
		[pPoleQueue removeAllObjects] ;		
	}
	
	
	
	NSMutableArray* pFirstPoleQueue = [m_pPole objectAtIndex:0] ;
	int countOfDiscus = [m_arrDiscurs count] ;
	for (int i=0; i<countOfDiscus; ++i) 
	{
		Discus* pDiscus = [m_arrDiscurs objectAtIndex:i] ;
		[pDiscus SetCurrPoleID:0] ;
		[pFirstPoleQueue addObject:pDiscus] ;
	}

	
}

-(void) DrawDiscus 
{
	for (int i=0; i < 3; ++i) 
	{
		NSMutableArray* pPoleQueue = [m_pPole objectAtIndex:i] ;
		int count = [pPoleQueue count] ;
		
		for (int j=0; j<count; ++j) 
		{
			Discus* pDiscus = [pPoleQueue objectAtIndex:j] ;
			if( nil == pDiscus )
			{
				break ;
			}
			
			CGPoint posOfDiscus = ccp( arrPosOfPole[i].x, arrPosOfPole[i].y + (15*j) ) ;
			pDiscus.position = posOfDiscus ;
		}
	}
	
	
	if( true == [self CheckSuccess] )
	{
		[self Finished] ;
		
		[[SoundFactory sharedSoundFactory] PlaydSound:@"levelup"] ;
		//PlayScene* playScene = [[PlayScene node] initWithDiscusNum:m_countOfDiscus+1] ;

	}
	
	
		
}


-(void) DrawInfoText 
{
	
	int myLevel = [[UserDataManager sharedUserDataManager] GetLevel];
	NSString* pStrLevelInfo = [NSString stringWithFormat:@"%d / %d",m_countOfDiscus, myLevel] ;
	if( nil == m_labelLevel )
	{
		m_labelLevel = [CCLabelTTF labelWithString:pStrLevelInfo fontName:@"Arial" fontSize:20] ;
		m_labelLevel.color = ccc3(255,255,255) ;
		m_labelLevel.position =  ccp( 110, 310 );
		[self addChild:m_labelLevel z:tagInfoText tag:tagInfoText];
	}
	else 
	{
		[m_labelLevel setString:pStrLevelInfo] ;
	}

	
	int result = pow(2,m_countOfDiscus) - 1 ;
	NSString* pStrMoveCount = [NSString stringWithFormat:@"%d / %d", m_moveCount, result] ;
	
	if( nil == m_labelMoveCount )
	{
		m_labelMoveCount = [CCLabelTTF labelWithString:pStrMoveCount fontName:@"Arial" fontSize:20] ;
		m_labelMoveCount.color = ccc3(255,255,255) ;
		m_labelMoveCount.anchorPoint = ccp(0, 0.5);
		m_labelMoveCount.position =  ccp( 210, 310 );
		[self addChild:m_labelMoveCount z:tagInfoText tag:tagInfoText];
	}
	else 
	{
		[m_labelMoveCount setString:pStrMoveCount] ;
	}
	

	
	
	
}


-(void) CountDown
{
	
	
	/*
	if( NO == [[UserDataManager sharedUserDataManager] GetCart])
	{
		int myLevel = m_countOfDiscus;
		if( myLevel > MAX_FREE_LEVEL )
		{
			[[SoundFactory sharedSoundFactory] PlaydSound:@"Cancel" volume:0.4] ;
			return ;
		}
	}
	 */
	
	if( m_isIng == NONE  )
	{
		
		if (m_isRakingMode) 
		{
			int minCoin = (pow((m_countOfDiscus-3+1),2))*10;
			int currCoin = [[UserDataManager sharedUserDataManager] GetCoin];
			if (currCoin < minCoin) 
			{
				return;
			}
			
			currCoin -= minCoin ;
			[[UserDataManager sharedUserDataManager] SetCoin:currCoin];
			
			NSString* pCoin = [NSString stringWithFormat:@"x%d", currCoin] ;
			if( nil == m_pCoinLabel )
			{
				m_pCoinLabel = [CCLabelTTF labelWithString:pCoin fontName:@"Arial" fontSize:20];
				m_pCoinLabel.color = ccc3(255,255,255) ;
				m_pCoinLabel.anchorPoint = ccp(1, 0.5);
				m_pCoinLabel.position =  ccp( 440, 290 );
				[self addChild:m_pCoinLabel z:tagInfoText tag:tagInfoText];
			}
			else 
			{
				[m_pCoinLabel setString:pCoin];
			}
			
		}

		
		
		m_isIng = COUNT_DOWN;
		
		
		id readyDelay = [CCDelayTime actionWithDuration:2.0] ;
		id countDelay = [CCDelayTime actionWithDuration:1.0] ;
		
		id callFunc_countDown = [CCCallFunc actionWithTarget:self selector:@selector(CountDown)] ;
		id actionToContDown = [CCSequence actions:callFunc_countDown, countDelay, nil];
		
		id countDown = [CCRepeat actionWithAction:actionToContDown  times:m_countDown+1];
		id callFunc_start = [CCCallFunc actionWithTarget:self selector:@selector(Start)] ;
		
		id delayStart = [CCDelayTime actionWithDuration:m_countDown];
		
		[self runAction:[CCSequence actions:readyDelay, countDown, nil]] ;
		[self runAction:[CCSequence actions:readyDelay, delayStart, callFunc_start, nil]] ;
		[[SoundFactory sharedSoundFactory] PlaydSound:@"FX0145"] ;
		
		if(m_imgCount)
		{
			[m_imgCount removeFromParentAndCleanup:NO];
		}
		
		m_imgCount = [CCSprite spriteWithFile:@"spr_ready.png"] ;
		m_imgCount.position =  ccp( 480/2, 200 );
		[self addChild:m_imgCount z:tagInfoText tag:tagInfoText];
		
	}
	else 
	{
		NSString* pCountString= nil;
		if( m_countDown == 0 )
		{
			pCountString = [NSString stringWithFormat:@"spr_go.png"];	
			[[SoundFactory sharedSoundFactory] PlaydSound:@"go"] ;
			
			
			id delay = [CCDelayTime actionWithDuration:1.0] ;
			id callFunc = [CCCallFunc actionWithTarget:self selector:@selector(RemoveCountLabel)] ;
			id action = [CCSequence actions:delay, callFunc, nil];
			
			[self runAction:action] ;
			
			
		}
		else 
		{
			pCountString = [NSString stringWithFormat:@"spr_%d.png", m_countDown];	
			[[SoundFactory sharedSoundFactory] PlaydSound:@"count_sec"] ;
		}
		
		[m_imgCount removeFromParentAndCleanup:NO];
		m_imgCount = [CCSprite spriteWithFile:pCountString];
		if( m_countDown )
		{
			m_imgCount.position =  ccp( 480/2, 200 );
			[self addChild:m_imgCount z:tagInfoText tag:tagInfoText];
		}
		
		
		if( m_countDown > 0 )
		{
			--m_countDown;
		}
	}
	
	m_imgCount.scale = 0.2;
	id scaleAction = [CCScaleTo actionWithDuration:0.5 scale:1.0] ;
	id fadeAction = [CCFadeIn actionWithDuration:0.5] ;
	[m_imgCount runAction:[CCSequence actions:scaleAction, fadeAction, nil]] ;


}

-(void) RemoveCountLabel
{
	[m_imgCount removeFromParentAndCleanup:NO];
}


-(Discus*) GetTopDiscus:(int)poleID
{
	NSMutableArray* pPoleQueue = [m_pPole objectAtIndex:poleID] ;
	Discus* pDiscus = [pPoleQueue lastObject] ;
	return pDiscus ;
	
}
	
-(bool) IsAbleToMoveDiscus:(Discus*)pDiscus toPoleID:(int)poleID 
{
	Discus* pTopDiscusAtDestPole = [self GetTopDiscus:poleID] ;
	if( [pDiscus GetDiscusID] < [pTopDiscusAtDestPole GetDiscusID] )
	{
		return false ;
	}
	
	
	return true ;
}

-(bool) AttachDiscusToPole:(Discus*)pDiscus toPoleID:(int)poleID 
{
	if( false == [self IsAbleToMoveDiscus:pDiscus toPoleID:poleID] )
	{
		return false; 
	}
	
	int currPoleID = [pDiscus GetCurrPoleID] ;
	if( currPoleID == poleID )
	{
		return false;
	}
	
	NSMutableArray* pCurrPoleQueue = [m_pPole objectAtIndex:currPoleID] ;
	[pCurrPoleQueue removeLastObject] ;
	
	[pDiscus SetCurrPoleID:poleID] ;
	NSMutableArray* pNextPoleQueue = [m_pPole objectAtIndex:poleID] ;
	[pNextPoleQueue addObject:pDiscus] ;
	
	++m_moveCount ;
	
	[self DrawDiscus] ;
	[self DrawInfoText] ;

	return true ;
}


-(void) SelectPole:(int)poleID  can:(bool)bIsAble
{
		
	
	if( poleID > -1 && poleID < 3)
	{
		if( bIsAble )
		{
			m_selectionMark.position = ccp( arrPosOfPole[poleID].x, 110) ;
			m_deselectionMark.position = ccp(500,500) ;
			[[SoundFactory sharedSoundFactory] PlaydSound:@"select"] ;
			
		}
		else 
		{
			m_deselectionMark.position = ccp( arrPosOfPole[poleID].x, 110) ;
			m_selectionMark.position = ccp(500,500) ;
			//[[SoundFactory sharedSoundFactory] PlaydSound:@"deselect"] ;
		}
		
	}
	else 
	{
		m_selectionMark.position = ccp(500,500) ;
		m_deselectionMark.position = ccp(500,500) ;
	}
	

}


-(bool) CheckSuccess 
{
	for (int i=0; i < 2; ++i) 
	{
		NSMutableArray* pPoleQueue = [m_pPole objectAtIndex:i] ;
		int count = [pPoleQueue count] ;
		if( count > 0 ) return false ;
		
	}
	
	NSMutableArray* pPoleQueue = [m_pPole objectAtIndex:2] ;
	int count = [pPoleQueue count] ;
	int underDiscusID = -1 ;
	for (int j=0; j<count; ++j) 
	{
		Discus* pDiscus = [pPoleQueue objectAtIndex:j] ;
		if( nil == pDiscus )
		{
			return false;
		}
		
		int currDiscusID = [pDiscus GetDiscusID] ;
		if ( currDiscusID < underDiscusID ) 
		{
			return false ;
		}
		
		underDiscusID = currDiscusID ;
	}
	
	return true ;
	
}

-(void) callbackOnPushed_homeMenuItem:(id) sender
{
	[[SoundFactory sharedSoundFactory] PlaydSound:@"FX0066" volume:0.3] ;
	
	MainScene* mainScene = [MainScene node] ;
	[[CCDirector sharedDirector] replaceScene:[CCTransitionFade transitionWithDuration:0.2 scene:mainScene] ] ;	//슬라이드

#ifdef AD_VER
	//[[UserDataManager sharedUserDataManager] viewAd:false];
#endif
}



-(void) callbackOnPushed_resetMenuItem:(id) sender
{
	if (PLAY!= m_isIng ) 
	{
		return;
	}
	
	[self ResetGame] ;
	[self DrawDiscus] ;
	[self DrawInfoText] ;
	
	[[SoundFactory sharedSoundFactory] PlaydSound:@"FX0070" volume:0.4] ;
}

-(void) callbackOnPushed_prevMenuItem:(id)sender
{
	if (m_isIng == COUNT_DOWN || m_isIng == PLAY) 
	{
		return ;
	}
	
	
	
	if( m_countOfDiscus > 3)
	{
		[[SoundFactory sharedSoundFactory] PlaydSound:@"FX0070" volume:0.4] ;
		m_countOfDiscus = m_countOfDiscus-1;
		PlayScene* playScene = [[PlayScene node] initWithDiscusNum:m_countOfDiscus mode:m_isRakingMode] ;
		[[CCDirector sharedDirector] replaceScene:playScene ] ;	//슬라이드

	}
	else {
		[[SoundFactory sharedSoundFactory] PlaydSound:@"Cancel" volume:0.4] ;
	}

	
}

-(void) callbackOnPushed_nextMenuItem:(id)sender
{
	
	if (m_isIng == COUNT_DOWN || m_isIng == PLAY) 
	{
		return ;
	}
	
	
	int myLevel = [[UserDataManager sharedUserDataManager] GetLevel];
    
    
#ifdef AD_VER
    bool bCart = [[UserDataManager sharedUserDataManager] GetCart];
    if( bCart == false )
    {
        if( m_countOfDiscus >= MAX_LEVEL_FOR_LITE_VERTION )
        {
            [[SoundFactory sharedSoundFactory] PlaydSound:@"Cancel" volume:0.4] ;
            
            id delay = [CCDelayTime actionWithDuration:0.0];
            id callFucn =[CCCallFunc actionWithTarget:self selector:@selector(LITE_VERSION_MessagePopup)] ;
            [self runAction:[CCSequence actions:delay, callFucn, nil] ];
            
            return;
        }
    }
#endif //AD_VER
    
	
	if ( false == m_isRakingMode) 
	{
		if( myLevel <= m_countOfDiscus )
		{
			[[SoundFactory sharedSoundFactory] PlaydSound:@"Cancel" volume:0.4] ;
			return;
		}
	}
	
	
		
	if( m_countOfDiscus < 10 )
	{
		[[SoundFactory sharedSoundFactory] PlaydSound:@"FX0070" volume:0.4] ;
		m_countOfDiscus = m_countOfDiscus+1;
		
		PlayScene* playScene = [[PlayScene node] initWithDiscusNum:m_countOfDiscus mode:m_isRakingMode] ;
		[[CCDirector sharedDirector] replaceScene:playScene ] ;	//슬라이드
		
	}
	else 
	{
		[[SoundFactory sharedSoundFactory] PlaydSound:@"Cancel" volume:0.4] ;
	}
	
	
	
	
	
}


-(void) callbackOnPushed_speakerMenuItem:(id)sender
{
	[[SoundFactory sharedSoundFactory] PlaydSound:@"FX0070" volume:0.4] ;
	if( NO ==[[UserDataManager sharedUserDataManager] GetSoundOpt] )
	{
		[[UserDataManager sharedUserDataManager] SetSoundOpt:YES];
		[self DrawMenu:YES];
		if( m_isIng == PLAY)
		{
			[[SoundFactory sharedSoundFactory] PlaydSound:@"BGM2" volume:0.0];
			
		}
	}
	else 
	{
		[[UserDataManager sharedUserDataManager] SetSoundOpt:NO];
		[self DrawMenu:NO];
		if( m_isIng == PLAY )
		{
			[[SoundFactory sharedSoundFactory] PlaydSound:@"BGM2"];
			
		}
	}

	
}


-(void) callbackLockBtn:(id)sender
{
	NSString *locale = [[NSLocale preferredLanguages] objectAtIndex:0];
	if ([locale isEqualToString:@"ko"]) 
	{
		NSString* recordMsg = [NSString stringWithFormat:@"6레벨 이상은 프로버젼에서 플레이 가능합니다.(레벨 3~10)"];
		MUIAlertView* alertView = [[[MUIAlertView alloc] 
									initWithTitle:@"하노이의탑 프로버젼" 
									message:recordMsg
									numberOfTextFields:0 
									delegate:self 
									cancelButtonTitle:@"취소" 
									otherButtonTitle:@"보기"] autorelease] ;
		[alertView show];
	}
	else 
	{
		NSString* recordMsg = [NSString stringWithFormat:@"You need to buy the 'Pro version' to play more level.(Level 3~10)"];
		MUIAlertView* alertView = [[[MUIAlertView alloc] 
									initWithTitle:@"Pro version" 
									message:recordMsg
									numberOfTextFields:0 
									delegate:self 
									cancelButtonTitle:@"Cancel" 
									otherButtonTitle:@"More"] autorelease] ;
		[alertView show];
		
	}
	
	
}


-(void) callbackNeedCoinBtn:(id)sender
{
	[[SoundFactory sharedSoundFactory] PlaydSound:@"FX0066" volume:0.3] ;
	
	BankScene* bankScene = [BankScene node] ;
	[[CCDirector sharedDirector] replaceScene:[CCTransitionFade transitionWithDuration:0.2 scene:bankScene] ] ;	//슬라이드
	
	
}



@end
