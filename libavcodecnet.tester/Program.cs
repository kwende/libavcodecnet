namespace libavcodecnet.tester
{
    class Program
    {
        static void Main(string[] args)
        {
            for (; ; )
            {
                using (Recorder recorder = Recorder.Create("test.mp4", 256, 256, 15, 12))
                {
                    byte[] frameBuffer = new byte[256 * 256 * 3];

                    for (int c = 0; c < 100000; c++)
                    {
                        for (int y = 0, i = 0; y < 256; y++)
                        {
                            for (int x = 0; x < 256; x++, i++)
                            {
                                frameBuffer[i * 3] = (byte)(y + x + c);
                                frameBuffer[i * 3 + 1] = (byte)(y + x + c);
                                frameBuffer[i * 3 + 2] = (byte)(y + x + c);
                            }
                        }

                        recorder.WriteFrame(frameBuffer);
                    }
                }
            }
        }
    }
}
