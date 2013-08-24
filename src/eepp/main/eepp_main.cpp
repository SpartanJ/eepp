
#if defined(_WIN32)

    #include <windows.h>

    extern int main(int argc, char* argv[]);

    int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, INT)
    {
        return main(__argc, __argv);
    }

#endif // _WIN32
