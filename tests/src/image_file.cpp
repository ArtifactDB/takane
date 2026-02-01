
TEST_F(SpatialExperimentTest, ImageSignatures) {
    takane::Options opt;
    const ritsuko::Version ver(1, 0, 0);

    {
        initialize_directory(dir);
        auto ipath = dir / "0.png";
        {
            std::ofstream ohandle(ipath);
        }
        expect_image_error("incomplete PNG file signature", dir, 0, "PNG", opt, ver);

        {
            std::ofstream ohandle(ipath);
            ohandle << "chino-chan";
        }
        expect_image_error("incorrect PNG file signature", dir, 0, "PNG", opt, ver);

        {
            std::ofstream ohandle(ipath);
            constexpr std::array<unsigned char, 8> stuff { 137, 80, 78, 71, 13, 10, 26, 10 };
            ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
        }
        takane::spatial_experiment::internal::validate_image(dir, 0, "PNG", opt, ver);
    }

    {
        initialize_directory(dir);
        auto ipath = dir / "1.tif";
        {
            std::ofstream ohandle(ipath);
        }
        expect_image_error("too small", dir, 1, "TIFF", opt, ver);

        {
            std::ofstream ohandle(ipath);
            ohandle << "chino-chan";
        }
        expect_image_error("incorrect TIFF file signature", dir, 1, "TIFF", opt, ver);

        {
            std::ofstream ohandle(ipath);
            constexpr std::array<unsigned char, 4> stuff{ 0x49, 0x49, 0x2A, 0x00 };
            ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
        }
        takane::spatial_experiment::internal::validate_image(dir, 1, "TIFF", opt, ver);

        {
            std::ofstream ohandle(ipath);
            constexpr std::array<unsigned char, 4> stuff{ 0x4D, 0x4D, 0x00, 0x2A };
            ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
        }
        takane::spatial_experiment::internal::validate_image(dir, 1, "TIFF", opt, ver);
    }

    const ritsuko::Version latestver(1, 3, 0);

    {
        initialize_directory(dir);
        auto ipath = dir / "2.jpg";
        {
            std::ofstream ohandle(ipath);
        }
        expect_image_error("not currently supported", dir, 2, "JPEG", opt, ver);
        expect_image_error("incomplete JPEG file signature", dir, 2, "JPEG", opt, latestver);

        {
            std::ofstream ohandle(ipath);
            ohandle << "chino-chan";
        }
        expect_image_error("incorrect JPEG file signature", dir, 2, "JPEG", opt, latestver);

        {
            std::ofstream ohandle(ipath);
            constexpr std::array<unsigned char, 4> stuff { 0xff, 0xd8, 0xff, 0xe1 };
            ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
        }
        takane::spatial_experiment::internal::validate_image(dir, 2, "JPEG", opt, latestver);
    }

    {
        initialize_directory(dir);
        auto ipath = dir / "3.gif";
        {
            std::ofstream ohandle(ipath);
        }
        expect_image_error("not currently supported", dir, 3, "GIF", opt, ver);
        expect_image_error("incomplete GIF file signature", dir, 3, "GIF", opt, latestver);

        {
            std::ofstream ohandle(ipath);
            ohandle << "chino-chan";
        }
        expect_image_error("incorrect GIF file signature", dir, 3, "GIF", opt, latestver);

        {
            std::ofstream ohandle(ipath);
            constexpr std::array<unsigned char, 4> stuff { 0x47, 0x49, 0x46, 0x38 };
            ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
        }
        takane::spatial_experiment::internal::validate_image(dir, 3, "GIF", opt, latestver);
    }

    {
        initialize_directory(dir);
        auto ipath = dir / "4.webp";
        {
            std::ofstream ohandle(ipath);
        }
        expect_image_error("not currently supported", dir, 4, "WEBP", opt, ver);
        expect_image_error("too small", dir, 4, "WEBP", opt, latestver);

        {
            std::ofstream ohandle(ipath);
            ohandle << "kirima-syaro";
        }
        expect_image_error("incorrect WEBP file signature", dir, 4, "WEBP", opt, latestver);

        {
            std::ofstream ohandle(ipath);
            constexpr std::array<unsigned char, 12> stuff { 0x52, 0x49, 0x46, 0x46, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
            ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
        }
        expect_image_error("incorrect WEBP file signature", dir, 4, "WEBP", opt, latestver);

        {
            std::ofstream ohandle(ipath);
            constexpr std::array<unsigned char, 12> stuff { 0x52, 0x49, 0x46, 0x46, 0x0, 0x0, 0x0, 0x0, 0x57, 0x45, 0x42, 0x50 };
            ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
        }
        takane::spatial_experiment::internal::validate_image(dir, 4, "WEBP", opt, latestver);
    }
}

