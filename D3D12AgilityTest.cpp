// D3D12AgilityTest.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include <iostream>
#include <cassert>
#include <string>
#include <utility>

#include "framework.h"
#include "D3D12AgilityTest.h"


#pragma region AgilitySDK

#include <dxgidebug.h>
#include <dxgi1_6.h>
#include "d3d12.h"

// デバイスの作成にlibを使用する
#define USE_D3D12_STATIC_LIB

// ID3D12SDKConfiguration::SetSDKVersionを利用してAgilitySDKを利用します。
// D3D12SDKVersion,D3D12SDKPathを定義する必要はありません。 
//#define USE_D3D12_SKD_CONFIGURATION

#pragma comment(lib, "dxgi.lib")


#ifdef USE_D3D12_STATIC_LIB
#   pragma comment(lib, "d3d12.lib")
#endif

/*
extern const で外部リンケージを持つ。
D3D12SDKVersion と D3D12SDKPath を定義することでAgilitySDKが発動する。
GetProcAddressは関数だけでなく変数のアドレスも返すようになっており、こちら側で定義した D3D12SDKVersion と D3D12SDKPath をd3d12.dllがGetProcAddressを介して取得しているようです。(関数でしか使ったことなくて失念してた)

また、こちら側で定義していなければd3d12.dllがデフォルトの処理を行うという仕組み。
*/

#ifndef USE_D3D12_SKD_CONFIGURATION

/**
 * @brief D3D12SDKVersion: アプリケーションが使用するAgilitySDKのD3D12Core.dllのSDKバージョン (2021/04/21現在は4) 
 *        バージョン情報: https://devblogs.microsoft.com/directx/directx12agility/
 * @note 4未満に設定するとデフォルトのD3D12Coreが呼び出されます。 現行バージョンの4より大きいとD3D12CreateDeviceが失敗します。
*/
extern "C" _declspec(dllexport) extern const UINT D3D12SDKVersion = 4;

/**
 * @brief D3D12SDKPath: D3D12Core.dllのディレクトリを指定します。 
 *        実行ファイル相対パスです。
 * @note utf8文字列の使用が必要です。 (c++20以降ではどのように管理するのだろうか)
*/
extern "C" _declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\";

#endif // !USE_D3D12_SKD_CONFIGURATION


#pragma endregion AgilitySDK


#define MAX_LOADSTRING 100

// グローバル変数:
HINSTANCE hInst;                                // 現在のインターフェイス
WCHAR szTitle[MAX_LOADSTRING];                  // タイトル バーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];            // メイン ウィンドウ クラス名

// このコード モジュールに含まれる関数の宣言を転送します:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE    hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR        lpCmdLine,
                     _In_ int           nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    // グローバル文字列を初期化する
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_D3D12AGILITYTEST, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // アプリケーション初期化の実行:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    // DirectX 12 Agility SKD の動作テスト
    // アプリケーション開発者がAgility SDKを利用することでDirectXの最新機能を利用するためにWindowsUpdateを行う必要がなくなります。
    // また、D3D12Core(ゲームに同梱させるdll)側に実装が移動したため、WindowsUpdateを行う必要がなくなるというメリットはゲームを遊ぶユーザーにも与えられます。
    {
        auto HR = [](HRESULT hr) { assert(SUCCEEDED(hr)); };

        // デバッグインターフェイスの有無を確認
        bool debug_enabled = false;
        {
            IDXGIDebug1* deb{};
            HR(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&deb)));
            if (deb)
            {
                debug_enabled = true;
                deb->Release(); 
                deb = nullptr;
            }
        }
    
    #ifndef USE_D3D12_STATIC_LIB
        HMODULE d3d12_module = LoadLibraryA("d3d12.dll");
        assert(d3d12_module);

        // 関数の取得
        PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(d3d12_module, "D3D12GetDebugInterface");
        PFN_D3D12_CREATE_DEVICE       D3D12CreateDevice      = (PFN_D3D12_CREATE_DEVICE)      GetProcAddress(d3d12_module, "D3D12CreateDevice");
        assert(D3D12GetDebugInterface && D3D12CreateDevice);
    #endif // !USE_D3D12_STATIC_LIB

        // デバッグレイヤの有効化
        {
            ID3D12Debug* debug_controller{};
            HR(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller))); // バージョン違いのAgilitySDKの場合D3D12CreateDevice以外も失敗する。
            if (debug_controller)
            {
                debug_controller->EnableDebugLayer();
                debug_controller->Release(); 
                debug_controller = nullptr;
            }
        }

    #ifdef USE_D3D12_SKD_CONFIGURATION
        /*
        D3D12SDKVersion 及び D3D12SDKPath を指定しなくても、
        ID3D12SDKConfiguration::SetSDKVersionから利用バージョンとSDKパスを設定できます。
        */

        PFN_D3D12_GET_INTERFACE D3D12GetInterface = (PFN_D3D12_GET_INTERFACE)GetProcAddress(d3d12_module, "D3D12GetInterface");
        assert(D3D12GetInterface);

        ID3D12SDKConfiguration* sdk_conf{};
        //HR(D3D12GetInterface(CLSID_D3D12SDKConfiguration, IID_PPV_ARGS(&sdk_conf)));
        HR(D3D12GetInterface(GUID{ 0x7cda6aca, 0xa03e, 0x49c8, 0x94, 0x58, 0x03, 0x34, 0xd2, 0x0e, 0x07, 0xce }, IID_PPV_ARGS(&sdk_conf)));

        HR(sdk_conf->SetSDKVersion(4, u8".\\D3D12\\")); // nullptrはNG
        sdk_conf->Release();
        sdk_conf = nullptr;

        D3D12GetInterface = nullptr;
    #endif // USE_D3D12_SKD_CONFIGURATION

        // デバイスを作成可能か検証します。
        {
            IDXGIFactory2* fa{};
            IDXGIAdapter1* ada{};
            HR(CreateDXGIFactory2(debug_enabled ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&fa)));
            HR(fa->EnumAdapters1(0, &ada));

            // D3D12SKDVersion=4 を設定すれば、D3D_FEATURE_LEVEL_12_2機能セットがサポートされることを確認しました。(Win20H2、リビジョン番号928、RTX2070、Geforceドライバ466.11以降)。
            // 4未満を設定するとシステムデフォルトのD3D12Coreが読み込まれます。
            ID3D12Device* dev{};
            if (FAILED(D3D12CreateDevice(ada, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&dev))))
                HR(D3D12CreateDevice(ada, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&dev)));
        
            dev->Release(); dev = nullptr;
            ada->Release(); ada = nullptr;
            fa->Release();  fa  = nullptr;
        }

    #ifndef USE_D3D12_STATIC_LIB
        // appendix
        HMODULE core = GetModuleHandleA("D3D12Core.dll"); // 自プロセスによってロードされたモジュールを取得できます。
        HMODULE me   = GetModuleHandleA(nullptr);         // nullptrを指定して自プロセスの作成に使用されたモジュールを取得します。
        core = NULL; // 返されたモジュールの参照カウントは増加しません。
        me   = NULL;

        FreeLibrary(d3d12_module);
        d3d12_module = NULL;
        D3D12GetDebugInterface = nullptr;
        D3D12CreateDevice      = nullptr;
    #endif // !USE_D3D12_STATIC_LIB
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_D3D12AGILITYTEST));

    MSG msg;

    // メイン メッセージ ループ:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_D3D12AGILITYTEST));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_D3D12AGILITYTEST);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // グローバル変数にインスタンス ハンドルを格納する

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND  - アプリケーション メニューの処理
//  WM_PAINT    - メイン ウィンドウを描画する
//  WM_DESTROY  - 中止メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 選択されたメニューの解析:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: HDC を使用する描画コードをここに追加してください...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// バージョン情報ボックスのメッセージ ハンドラーです。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
