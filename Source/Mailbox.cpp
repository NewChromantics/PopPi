#include "Mailbox.h"
#include "Kernel.h"


static_assert(sizeof(uint32_t*) == sizeof(uint32_t), "Expecting pointers to be 32bit for mailbox data");


//	https://www.raspberrypi.org/forums/viewtopic.php?f=72&t=67970
typedef struct
{
	unsigned int read;		//	0
	unsigned int unused1;
	unsigned int unused2;
	unsigned int unused3;
	unsigned int poll;		//	16
	unsigned int sender;	//	20
	unsigned int status;	//	24
	unsigned int configuration;	//	28
	unsigned int write;		//	32
} Mailbox;

static volatile Mailbox* const MAILBOX0 = (Mailbox*)0x2000b880;




//	some more references for magic nmbers
//	https://www.raspberrypi.org/forums/viewtopic.php?f=29&t=65596
//	http://magicsmoke.co.za/?p=284
#define MAILBOX_STATUS_BUSY		0x80000000
#define MAILBOX_STATUS_EMPTY	0x40000000


void TMailbox::Write(volatile void* Data,TMailbox::TChannel Channel)
{
	Data = TKernel::GetGpuAddress( Data );
	uint32_t BufferAddress = (uint32_t)Data;
	
	auto ChannelBitMask = (1<<4)-1;
	if ( BufferAddress & ChannelBitMask )
	{
		//	throw
		BufferAddress &= ~ChannelBitMask;
	}

	
	//	and bit 1 to say... we're writing it? or because it's the channel?..
	//	odd that it'll overwrite the display widht, but maybe that has to be aligned or something anyway
	//	4bits per channel
	//	gr: could be AllocateBuffer command; http://magicsmoke.co.za/?p=284
	//	gr: why arent we setting all these tags? http://magicsmoke.co.za/?p=284
	BufferAddress |= static_cast<uint32_t>(Channel);

    while(1)
    {
		unsigned int mailbox = 0x2000B880;
		//	https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
		//	0x80000000: request successful
		//	0x80000001: error parsing request buffer (partial response)
		uint32_t MailboxResponse = GET32(mailbox+24);
		//uint32_t MailboxResponse = MAILBOX0->sender;
		//uint32_t MailboxResponse = GET32( (uint32_t)&MAILBOX0->sender );
		
		//	break when we... have NO response waiting??
		bool StatusBusy = bool_cast(MailboxResponse & MAILBOX_STATUS_BUSY);
		if( !StatusBusy )
			break;
    }
	
	//	gr: where is
	
	//	gr: I bet this is configuration
	MAILBOX0->write = BufferAddress;
}



uint32_t TMailbox::Read(TMailbox::TChannel Channel)
{
	auto ChannelBitMask = (1<<4)-1;
    unsigned int mailbox = 0x2000B880;
    while(1)
    {
        while(1)
        {
			//	gr: sender, not status, suggests this struct may be off...
			//uint32_t MailboxResponse = GET32( (uint32_t)&MAILBOX0->sender );
			uint32_t MailboxResponse = GET32( mailbox+24 );
			bool StatusEmpty = bool_cast(MailboxResponse & MAILBOX_STATUS_EMPTY);
            if ( !StatusEmpty )
				break;
        }

		//	gr: switching this to MAILBOX0->read doesn't work :/
		auto Response = GET32( mailbox+0 );
		//auto Response = GET32( (uint32_t)&MAILBOX0->read );
		auto ResponseChannel = static_cast<TMailbox::TChannel>( Response & ChannelBitMask );
        if ( ResponseChannel == Channel )
		{
			Response &= ~ChannelBitMask;
			return Response;
		}
    }
	
	//	throw
	return -1;
}

bool TMailbox::EnableQpu(bool Enable)
{
	uint32_t Data[7] __attribute__ ((aligned(16)));
	static_assert( sizeof(Data) == 4*7, "sizeof");
	
	//	header
	Data[0] = sizeof(Data);
	Data[1] = 0x00000000;	//	process request (0 = writing mailbox)
	Data[2] = 0x00030012;	//	the tag id
	Data[3] = 4;			//	size of the buffer
	Data[4] = 4;			//	size of the data
	
	//	buffer here
	Data[5] = Enable ? 1 : 0;
	
	//	footer
	Data[6] = 0x00000000;	// end tag
	
	//	gr: channel 1 doesn't boot properly (screen resets?)
	//	https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
	//	8 is supposed to be ARM to VC
	auto Channel = TMailbox::TChannel::Gpu;
	TMailbox::Write( Data, Channel );
	//	wait for it to finish
	TMailbox::Read( Channel );
	
	//mbox_property(file_desc, p);
	//return p[5];
	auto Enabled = (Data[5] == 1);
	return Enabled;
}





volatile uint32_t MailboxBuffer[1000*1000] __attribute__ ((aligned(16)));
int MailboxBufferPos = 1;

volatile uint32_t* TMailbox::AllocBuffer(int Size)
{
	//	align to 16 bytes
	int Align = 16/sizeof(uint32_t);
	if ( MailboxBufferPos % Align != 0 )
	{
		MailboxBufferPos += Align - (MailboxBufferPos % Align);
	}
	auto* Address = &MailboxBuffer[MailboxBufferPos];
	MailboxBufferPos += Size;
	return Address;
}

