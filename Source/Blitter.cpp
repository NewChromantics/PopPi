#include "Blitter.h"
#include "Sprites.h"

#define TEST_PAD	100


TBlitter::TBlitter(std::function<TCanvas<TPixel>()> LockCanvas) :
	mLockCanvas	( LockCanvas )
{
	
}

TCanvas<TBlitter::TPixel> TBlitter::GetCanvas()
{
	return mLockCanvas();
}

void TBlitter::SetPixel(int Index,uint32_t Colour)
{
	auto Canvas = GetCanvas();

	Canvas.mPixels[Index] = Colour;
}


void TBlitter::SetPixel(int x,int y,uint32_t Colour)
{
	auto Canvas = GetCanvas();
	
	auto Index = Canvas.GetIndex(x,y);
	Canvas.mPixels[Index] = Colour;
}


void TBlitter::FillRow(int y,uint32_t Colour)
{
	auto Canvas = GetCanvas();

	auto Start = Canvas.GetIndex( 0, y );
	auto End = Canvas.GetIndex( Canvas.mWidth, y );
	
	for ( ;	Start<End;	Start++ )
	{
		Canvas.mPixels[Start] = Colour;
	}
}




void TBlitter::DrawChar(int x,int y,int Char,int& Width)
{
	auto Canvas = GetCanvas();

	int Height;
	auto* Sprite = PopSprite::GetSprite( Char, Width, Height );

	//	auto wrap here as we can't with GetConsoleXY any more as we don't have width/height info
	if ( x + Width >= Canvas.mWidth )
	{
		x %= Canvas.mWidth;
		y += Height + GetLineSpacing();
	}
	y %= Canvas.mHeight;
		

	for ( int row=0;	row<Height;	row++ )
	{
		for ( int col=0;	col<Width;	col++ )
		{
			auto px = x + col;
			auto py = y + row;
			auto ColourIndex = Sprite[col + (row*Width)];
			auto Colour = PopSprite::GetPaletteColour(ColourIndex);
			if ( !Colour.IsOpaque() )
				continue;
			SetPixel( px,py,Colour.bgra );
		}
	}
	
	//	update console pos
	mConsolePos.y = y;
	mConsolePos.x = x + Width;
}


void TBlitter::DrawNumber(int x,int y,uint32_t Number)
{
	int DigitsReversed[20+TEST_PAD];
	DigitsReversed[0] = 0;
	int DigitCount = 0;
	if ( Number == 0 )
	DigitsReversed[DigitCount++] = '0';
	while ( Number > 0 && DigitCount < 20 )
	{
		DigitsReversed[DigitCount++] = (Number % 10) + '0';
		Number /= 10;
	};
	
	
	char Digits[20+TEST_PAD];
	for ( int i=DigitCount-1;  i>=0;  i-- )
	{
		auto di = DigitCount - 1 - i;
		Digits[di] = DigitsReversed[i];
	}
	Digits[DigitCount] = '\0';
	DrawString( x, y, Digits );
}


void TBlitter::DrawHex(int x,int y,uint32_t Number)
{
	auto Canvas = GetCanvas();

	char Digits[11 + TEST_PAD];
	Digits[0] = '0';
	Digits[1] = '*';
	for ( int i=0;	i<8;	i++ )
	{
		int d = 2 + i;
		int Shift = (7-i) * 4;
		int h = (Number >> Shift) & 0xf;
		if ( h >= 10 )
		Digits[d] = (h-10) + 'a';
		else
		Digits[d] = (h) + '0';
	}
	Digits[10] = '\0';
	DrawString( x, y, Digits );
}

void TBlitter::DrawString(int x,int y,const char* String)
{
	auto Canvas = GetCanvas();

	
	int MaxSize = 400;
	while ( MaxSize-- > 0 )
	{
		auto Char = String[0];
		String++;
		if ( Char == '\0' )
		break;
		int Width = 1;
		DrawChar( x, y, Char, Width );
		x += Width;
	}
}

void TBlitter::FillPixels(uint32_t Colour)
{
	auto Canvas = GetCanvas();
	
	auto Start = Canvas.GetIndex( 0, 0 );
	auto End = Canvas.GetIndex( Canvas.mWidth, Canvas.mHeight );
	
	for ( ;	Start<End;	Start++ )
	{
		Canvas.mPixels[Start] = Colour;
	}
}

void TBlitter::FillPixelsCheckerBoard(int SquareSize,uint32_t ColourA,uint32_t ColourB)
{
	auto Canvas = GetCanvas();
	
	uint32_t Colours[2+TEST_PAD];
	Colours[0] = ColourA;
	Colours[1] = ColourB;
	
	auto* Pixels = Canvas.mPixels;
	for ( unsigned y=0;	y<Canvas.mHeight;	y++ )
	{
		auto yodd = (y/SquareSize) & 1;
		for ( unsigned x=0;	x<Canvas.mWidth;	x++ )
		{
			auto xodd = (x/SquareSize) & 1;
			if ( yodd )
			xodd = !xodd;
			auto ColourIndex = xodd;
			auto p = Canvas.GetIndex(x,y);
			Pixels[p] = Colours[ColourIndex];
		}
	}
}



int TBlitter::GetConsoleY()
{
	return mConsolePos.y;
}

int TBlitter::GetConsoleX(bool NewLine)
{
	if ( NewLine )
	{
		mConsolePos.y += 10;
		mConsolePos.x = 1;
	}
	
	/*
	auto Right = mWidth - 10;
	
	//	wrap
	if ( mConsoleX >= Right )
	{
		mConsoleX = 1;
		mConsoleY += 10;
	}
	*/
	return mConsolePos.x;
}

