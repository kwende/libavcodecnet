namespace libavcodecnet.tester
{
    class Program
    {
        static void TestInParallel()
        {
            const int Width = 512, Height = 424;

            for (; ; )
            {
                int fileCount = 0;
                Parallel.For(0, 6, (p) =>
                {
                    int valueToUse = Interlocked.Increment(ref fileCount);
                    Console.WriteLine($"Iteration {valueToUse} running.");
                    using (Recorder recorder = Recorder.Create($"test_{valueToUse}.mp4", Width, Height, 15, 12))
                    {
                        byte[] frameBuffer = new byte[Width * Height * 3];

                        for (int c = 0; c < 100000; c++)
                        {
                            for (int y = 0, i = 0; y < Width; y++)
                            {
                                for (int x = 0; x < Height; x++, i++)
                                {
                                    frameBuffer[i * 3] = (byte)(y + x + c);
                                    frameBuffer[i * 3 + 1] = (byte)(y + x + c);
                                    frameBuffer[i * 3 + 2] = (byte)(y + x + c);
                                }
                            }

                            recorder.WriteFrame(frameBuffer);
                        }
                    }
                    //}
                });
            }
        }

        static void Recorder2()
        {
            const int Width = 512, Height = 424;

            using (Recorder16 recorder = Recorder16.Create($"test.mp4", Width, Height, 15, 1))
            {
                ushort[] frameBuffer = new ushort[Width * Height];

                for (int c = 0; c < 1000; c++)
                {
                    for (int y = 0, i = 0; y < Width; y++)
                    {
                        for (int x = 0; x < Height; x++, i++)
                        {
                            frameBuffer[i] = 40000; // (ushort)(y * x + c * 7000);
                        }
                    }

                    //recorder.WriteFrame(frameBuffer);
                    byte[] pngBuffer = recorder.WriteAndReturnFrame(frameBuffer);

                    //File.WriteAllBytes("C:/users/brush/desktop/fartturd.png", pngBuffer);
                    Console.Write(".");
                }
            }
        }

        static void Recorder3()
        {
            const string inputPath = @"E:\2023_01_24_tray_V4_VOC\JPEGImages_orig\0000001-6a9a80d9-53df-44eb-a751-d9241c14dcb2-1.png";
            const string outputPath = "C:/users/brush/desktop/changedpoop.png";

        }

        static void Main(string[] args)
        {
            //h265Converter converter = new h265Converter();
            //converter.Initialize(512, 512, 17);

            //ushort[] frameBuffer = new ushort[512 * 512];
            //for (int c = 0; ; c++)
            //{
            //    converter.EncodeAndDecodeDepth(frameBuffer);
            //}


            ColorSpaceConverter converter = new ColorSpaceConverter();

            string[] files = Directory.GetFiles(@"E:\2023_01_24_tray_V4_VOC\VOC2007\JPEGImages");

            DateTimeOffset start = DateTimeOffset.Now;
            TimeSpan elapsedTime = TimeSpan.MinValue;
            for (int c = 0; c < files.Length; c++)
            {

                string file = files[c];
                if (c % 10 == 0 && elapsedTime > TimeSpan.MinValue)
                {
                    Console.Clear();
                    double filesPerSecond = elapsedTime.TotalSeconds / (c * 1.0 + 1);
                    int filesLeft = files.Length - c + 1;
                    double secondsLeft = filesPerSecond * filesLeft;
                    Console.WriteLine($"{c}/{files.Length}, should be done at {DateTimeOffset.Now.AddSeconds(secondsLeft)}");
                }
                string destination = Path.Combine(@"E:\2023_01_24_tray_V4_VOC\VOC2007_h265_17\JPEGImages", Path.GetFileName(file));
                converter.Convert16Bit2H265PNG(file, 512, 512, 17, destination);
                elapsedTime = DateTimeOffset.Now - start;
            }

            //https://stackoverflow.com/questions/66155414/convert-16bit-grayscale-png-to-hevc-x265
            //Recorder3();

            //string[] files = Directory.GetFiles(@"E:\2023_01_24_tray_V4_VOC\VOC2007\JPEGImages");
            //for (int c = 0; c < files.Length; c++)
            //{
            //    string file = files[c];

            //    if (c % 100 == 0)
            //    {
            //        Console.Clear();
            //        Console.WriteLine($"{c}/{files.Length}");
            //    }

            //    ColorSpaceConverter converter = new ColorSpaceConverter();

            //    string destinationName = Path.Combine(@"E:\2023_01_24_tray_V4_VOC\VOC2007\JPEGImages2", Path.GetFileName(file));

            //    converter.Convert16Bit2YChannelPNG(file,
            //        512, 512,
            //        destinationName);
            //}

            //ColorSpaceConverter cs = new ColorSpaceConverter();

            //byte[] data = File.ReadAllBytes("c:/users/brush/desktop/shit.dat");
            //ushort[] ushorts = new ushort[512 * 424];

            //Buffer.BlockCopy(data, 0, ushorts, 0, data.Length);

            //cs.Save16BitYChannelPNG(ushorts, 512, 424, "c:/users/brush/desktop/test.png");
        }
    }
}
