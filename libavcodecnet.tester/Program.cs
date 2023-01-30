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
                            frameBuffer[i] = (ushort)(y * x + c * 7000);
                        }
                    }

                    recorder.WriteFrame(frameBuffer);
                    Console.Write(".");
                }
            }
        }

        static void Main(string[] args)
        {
            //https://stackoverflow.com/questions/66155414/convert-16bit-grayscale-png-to-hevc-x265
            //Recorder2();

            foreach (string file in Directory.GetFiles(@"C:\Users\brush\Desktop\png test"))
            {
                ColorSpaceConverter converter = new ColorSpaceConverter();

                string destinationName = Path.Combine(Path.GetDirectoryName(file), Path.GetFileNameWithoutExtension(file) + "_converted.png");

                converter.Convert16Bit2YChannelPNG(@"C:\Users\brush\Desktop\png test\0000001-6a9a80d9-53df-44eb-a751-d9241c14dcb2-1.png",
                    512, 512,
                    destinationName);
            }

            //ColorSpaceConverter cs = new ColorSpaceConverter();

            //byte[] data = File.ReadAllBytes("c:/users/brush/desktop/shit.dat");
            //ushort[] ushorts = new ushort[512 * 424];

            //Buffer.BlockCopy(data, 0, ushorts, 0, data.Length);

            //cs.Save16BitYChannelPNG(ushorts, 512, 424, "c:/users/brush/desktop/test.png");
        }
    }
}
