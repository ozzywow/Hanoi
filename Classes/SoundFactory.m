//
//  SoundFactory.m
//  KW
//
//  Created by Jinguk Hong on 10. 3. 2..
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "SoundFactory.h"
#import "UserDataManager.h"



@implementation SoundFactory



static SoundFactory*		_soundFactory = nil;
-(id) init
{
	if ([super init] != nil) 
	{
		m_soundPool = [[NSMutableDictionary alloc] init] ;
		return self ;
	}
	
	return nil ;
}


-(AVAudioPlayer*) PlaydSound :(NSString*) soundName
{
	return [self PlaydSound:soundName volume:MAST_VOLUME];
}

-(AVAudioPlayer*) PlaydSound :(NSString*) soundName volume:(float)vol
{
	
	NSString* path= nil;
	AVAudioPlayer* pSound = [m_soundPool objectForKey:soundName] ;
	if( nil == pSound )
	{
		
		path = [[NSBundle mainBundle] pathForResource:soundName ofType:@"wav"] ; 
		if (path) 
		{
			pSound = [[AVAudioPlayer alloc ]initWithContentsOfURL:[NSURL fileURLWithPath:path] error:nil ] ;
			if( pSound )
			{
				
				[m_soundPool setObject:pSound forKey:soundName] ;
			}
		}
	}
	
	
	if( nil == pSound )
	{
		path = [[NSBundle mainBundle] pathForResource:soundName ofType:@"mp3"] ; 
		if (path) 
		{
			pSound = [[AVAudioPlayer alloc ]initWithContentsOfURL:[NSURL fileURLWithPath:path] error:nil ] ;
			if( pSound )
			{
			
				[m_soundPool setObject:pSound forKey:soundName] ;
			}
		}
		
	}
	
	
	if( nil == pSound )
	{
		path = [[NSBundle mainBundle] pathForResource:soundName ofType:@"m4a"] ; 
		if (path) 
		{
			pSound = [[AVAudioPlayer alloc ]initWithContentsOfURL:[NSURL fileURLWithPath:path] error:nil ] ;
			if( pSound )
			{
				[m_soundPool setObject:pSound forKey:soundName] ;
			}
		}
		
	}
	
	
	if( pSound )
	{
		
		if( NO ==[[UserDataManager sharedUserDataManager] GetSoundOpt] )
		{
			if( pSound.playing )
			{
				[pSound stop] ;
				pSound.currentTime = 0 ;
			}
			[pSound setVolume:vol] ;
			[pSound play] ;
		}
		
		
	}
	else
	{
		NSLog(@"Can not found any sound file (%@)",soundName ) ;
	}
	
	
	return pSound ;
}


-(void) StopSound :(NSString*) soundName 
{
	AVAudioPlayer* pSound = [m_soundPool objectForKey:soundName] ;
	if( pSound )
	{
		[pSound stop] ;
		pSound.currentTime = 0 ;
	}
	else
	{
		NSLog(@"Can not found any sound file (%@)",soundName ) ;
	}
}




-(void) dealloc
{
	[m_soundPool release] ;
	[super dealloc] ;
}



+ (SoundFactory *)sharedSoundFactory
{
	@synchronized([SoundFactory class])
	{
		if (!_soundFactory) {
			
			//
			// Default Director is TimerDirector
			// 
			if( [ [SoundFactory class] isEqual:[self class]] )
				_soundFactory = [[SoundFactory alloc] init];
			else
				_soundFactory = [[self alloc] init];
		}
		
	}
	return _soundFactory;
}





@end
