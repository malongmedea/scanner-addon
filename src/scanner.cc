#include <napi.h>
#include <tchar.h>
#include <string>
#include <windows.h>
#include "twain.h"

bool SaveBitmapToFile(HBITMAP hBitmap, char *szfilename);

Napi::String InitDll(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  HWND hwnd; // = FindWindow("ConsoleWindowClass", NULL);
  std::string stateMsg;
  stateMsg = "";
  //��ȡ���ھ��
  if (info.Length() > 0) {
    if (info[0].IsBuffer()) {
      Napi::Object bufferObj = info[0].As<Napi::Object>().ToObject();
      // double arg0 = info[0].As<Napi::Number>().DoubleValue();
      if (bufferObj == NULL) {
        return Napi::String::New(env, "error-buffer-to-object");
      } else {
        // return Napi::String::New(env, "success-buffer-to-object");
      }
      stateMsg += "success-buffer-to-object<br>";

      unsigned char *bufferData =
          (unsigned char *)bufferObj.As<Napi::Buffer<char>>().Data();
      unsigned long handle = *reinterpret_cast<unsigned long *>(bufferData);
      hwnd = (HWND)handle; // �õ����ھ��
      if (hwnd == NULL) {
        stateMsg += "error-get-window-handle<br>";
        return Napi::String::New(env, stateMsg);
      }
      // ��֤���ھ���Ƿ��ȡ�ɹ�����ȷ
      // MoveWindow(hwnd,0,0,1000,900,false);
    }
  } else {
    stateMsg += "error-parameter<br>";
    return Napi::String::New(env, stateMsg);
  }
  /************************************************************************/
  /* STATE 1 to 2: Load the Source manager and Get the DSM_Entry */
  /************************************************************************/
  // ��̬��
  HMODULE hDLL;
  //���ض�̬��
  hDLL = LoadLibrary(_T("twain_32.dll"));
  if (hDLL == NULL) {
    return Napi::String::New(env, "error-dll-load");
  }

  //����ָ��
  DSMENTRYPROC fnDSMEntry;
  fnDSMEntry = (DSMENTRYPROC)GetProcAddress(hDLL, "DSM_Entry");
  //���ָ��Ϊ�գ��ͷŵ����
  if (fnDSMEntry == NULL) {
    FreeLibrary(hDLL);
    hDLL = NULL;
    return Napi::String::New(env, "error-twain-DSMEntry");
  }
  /************************************************************************/
  /* STATE 2 TO 3 ��Open the Source Manager                               */
  /************************************************************************/
  stateMsg += "success-get-window-handle<br>";
  // return Napi::String::New(env, "success-get-window-handle");

  TW_IDENTITY twApp;
  twApp.Id = 0;
  strcpy_s(twApp.Manufacturer, sizeof(TW_STR32), "karl.ma");
  strcpy_s(twApp.ProductFamily, sizeof(TW_STR32), "Demo");
  strcpy_s(twApp.ProductName, sizeof(TW_STR32), "TwainScanner");
  twApp.ProtocolMajor = TWON_PROTOCOLMAJOR;
  twApp.ProtocolMinor = TWON_PROTOCOLMINOR;
  twApp.SupportedGroups = DG_IMAGE | DG_CONTROL;
  twApp.Version.Country = TWCY_CHINA;
  twApp.Version.Language = TWLG_CHINESE;
  twApp.Version.MajorNum = 0;
  twApp.Version.MinorNum = 0;
  strcpy_s(twApp.Version.Info, sizeof(TW_STR32), "TwainScanner 0.0.0");

  // ����һ��״̬��ʶ
  TW_UINT16 nRet;
  nRet =
      fnDSMEntry(&twApp, NULL, DG_CONTROL, DAT_PARENT, MSG_OPENDSM, &hwnd);
  // If success then goto state 3
  if (nRet == TWRC_SUCCESS) {
    stateMsg += "success-open-DSM<br>";
    // return Napi::String::New(env, "success-open-DSM");
    TW_IDENTITY twDest;
    twDest.Id = 0;

    /************************************************************************/
    /* STATE 3 ��Select the Source */
    /************************************************************************/
    nRet = fnDSMEntry(&twApp, NULL, DG_CONTROL, DAT_IDENTITY,
                      MSG_USERSELECT, &twDest);
    // nRet =
    // fnDSMEntry(&twApp,NULL,DG_CONTROL,DAT_IDENTITY,MSG_GETDEFAULT,&twDest);
    /************************************************************************/
    /* STATE 3 to 4 ��Open the Source */
    /************************************************************************/
    nRet = fnDSMEntry(&twApp, NULL, DG_CONTROL, DAT_IDENTITY, MSG_OPENDS,
                      &twDest);
    if (nRet == TWRC_SUCCESS) {
      stateMsg += "success-open-source<br>";
      /************************************************************************/
      /* STATE 4 */
      /************************************************************************/
      TW_CAPABILITY twCap;
      twCap.Cap = ICAP_XFERMECH;
      twCap.ConType = TWON_ONEVALUE;
      twCap.hContainer = GlobalAlloc(GHND, sizeof(TW_ONEVALUE));
      pTW_ONEVALUE pValue = (pTW_ONEVALUE)GlobalLock(twCap.hContainer);
      pValue->Item = TWTY_UINT16;
      pValue->ItemType = TWSX_NATIVE;
      GlobalUnlock(twCap.hContainer);
      nRet = fnDSMEntry(&twApp, &twDest, DG_CONTROL, DAT_CAPABILITY,
                        MSG_SET, &twCap);
      GlobalFree(twCap.hContainer);
      /************************************************************************/
      /* STATE 4 to 5 */
      /************************************************************************/
      TW_USERINTERFACE twUI;
      twUI.hParent = hwnd;
      twUI.ModalUI = 0;
      twUI.ShowUI = TRUE;

      nRet = fnDSMEntry(&twApp, &twDest, DG_CONTROL, DAT_USERINTERFACE,
                        MSG_ENABLEDS, &twUI);
      stateMsg += "STATE 4 to 5<br>";
      /************************************************************************/
      /* STATE 5 to 6 */
      /************************************************************************/
      if (nRet == TWRC_SUCCESS) {
        stateMsg += "success-source-enabled<br>";

        TW_EVENT twEvent;
        MSG msg;
        BOOL bLoop = TRUE;
        BOOL bRet;
        // GetMessage -> winUser.h
        while (bRet = GetMessage(&msg, NULL, 0, 0) != 0 && bLoop) {
          if (bRet == -1) {
            DWORD wrd = GetLastError();
            wrd;
            break;
          }
          twEvent.pEvent = &msg;
          twEvent.TWMessage = MSG_NULL;
          nRet = fnDSMEntry(&twApp, &twDest, DG_CONTROL, DAT_EVENT,
                            MSG_PROCESSEVENT, &twEvent);
          if (nRet == TWRC_DSEVENT) {
            switch (twEvent.TWMessage) {
              case MSG_XFERREADY: {
                stateMsg += "success-MSG_XFERREADY<br>";
                /************************************************************************/
                /* STATE 6 to 7 */
                /************************************************************************/
                HBITMAP hBitmap;
                int count = 0;
                // TCHAR szFilename[_MAX_PATH];
                TW_PENDINGXFERS twPx;
                while (TRUE) {
                  count++;
                  nRet = fnDSMEntry(&twApp, &twDest, DG_IMAGE,
                                    DAT_IMAGENATIVEXFER, MSG_GET, &hBitmap);
                  // {
                  // //   _stprintf_s(szFilename, _MAX_PATH,
                  // //               _T("D:\\temp\\%04d.bmp"), count);
                  // //   if (SaveBitmapToFile(hBitmap, szFilename)) {
                  // //     stateMsg = "success-write-file";
                  // //   } else {
                  // //     stateMsg = "error-write-file";
                  // //   }
                  
                  //   stateMsg += "success-write-file<br> ";
                  // }
                  stateMsg += "success-hBitmap<br>";
                  GlobalFree(hBitmap);

                  /************************************************************************/
                  /* STATE 7 to 6 to 5 */
                  /************************************************************************/
                  nRet = fnDSMEntry(&twApp, &twDest, DG_CONTROL,
                                    DAT_PENDINGXFERS, MSG_ENDXFER, &twPx);
                  if (nRet != TWRC_SUCCESS)
                    break;
                  if (twPx.Count == 0)
                    break;
                }
                bLoop = FALSE;
                break;
              }
              case MSG_CLOSEDSOK:
              case MSG_CLOSEDSREQ:
                bLoop = FALSE;
                break;

              default:
                // winUser.h
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                break;
              }
          } else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
          }
        }

        // State 5 to 4
        fnDSMEntry(&twApp, &twDest, DG_CONTROL, DAT_USERINTERFACE,
                    MSG_DISABLEDS, &twUI);
      }
      // State 4 to 3
      fnDSMEntry(&twApp, NULL, DG_CONTROL, DAT_IDENTITY, MSG_CLOSEDS,
                  &twDest);
    } else if (nRet == TWRC_FAILURE) {
      stateMsg += "error-open-source<br>";
      //ʧ���ˣ���ȡ״̬
      TW_STATUS twStatus;
      nRet = fnDSMEntry(&twApp, NULL, DG_CONTROL, DAT_IDENTITY, MSG_GET,
                        &twStatus);
    }

    stateMsg += "State 3 to 2<br>";
    // State 3 to 2
    // Close the DMS
    nRet = fnDSMEntry(&twApp, NULL, DG_CONTROL, DAT_PARENT, MSG_CLOSEDSM,
                      &hwnd);
  } else {
    stateMsg += "error-open-DSM<br>";
  }
  stateMsg += "State 2 to 1<br>";
  // State 2 to 1
  FreeLibrary(hDLL);
  hDLL = NULL;
    
  return Napi::String::New(env, stateMsg);
}

bool SaveBitmapToFile(HBITMAP hBitmap, char *szfilename) {
  HDC hdc; //�豸������
  int ibits;
  WORD wbitcount; //��ǰ��ʾ�ֱ�����ÿ��������ռ�ֽ���

  //λͼ��ÿ��������ռ�ֽ����������ɫ���С��λͼ�������ֽڴ�С��λͼ�ļ���С
  //��д���ļ��ֽ���
  DWORD dwpalettesize = 0, dwbmbitssize, dwdibsize, dwwritten;

  BITMAP bitmap;           //λͼ���Խṹ
  BITMAPFILEHEADER bmfhdr; //λͼ�ļ�ͷ�ṹ
  BITMAPINFOHEADER bi;     //λͼ��Ϣͷ�ṹ
  LPBITMAPINFOHEADER lpbi; //ָ��λͼ��Ϣͷ�ṹ

  //�����ļ��������ڴ�������ɫ����
  HANDLE fh, hdib, hpal, holdpal = NULL;

  //����λͼ�ļ�ÿ��������ռ�ֽ���
  hdc = CreateDC("display", NULL, NULL, NULL);
  ibits = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);
  DeleteDC(hdc);

  if (ibits <= 1)
    wbitcount = 1;
  else if (ibits <= 4)
    wbitcount = 4;
  else if (ibits <= 8)
    wbitcount = 8;
  else if (ibits <= 16)
    wbitcount = 16;
  else if (ibits <= 24)
    wbitcount = 24;
  else
    wbitcount = 32;

  //�����ɫ���С
  if (wbitcount <= 8)
    dwpalettesize = (1 << wbitcount) * sizeof(RGBQUAD);

  //����λͼ��Ϣͷ�ṹ
  GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bitmap);
  bi.biSize = sizeof(BITMAPINFOHEADER);
  bi.biWidth = bitmap.bmWidth;
  bi.biHeight = bitmap.bmHeight;
  bi.biPlanes = 1;
  bi.biBitCount = wbitcount;
  bi.biCompression = BI_RGB;
  bi.biSizeImage = 0;
  bi.biXPelsPerMeter = 0;
  bi.biYPelsPerMeter = 0;
  bi.biClrUsed = 0;
  bi.biClrImportant = 0;

  dwbmbitssize = ((bitmap.bmWidth * wbitcount + 31) / 32) * 4 * bitmap.bmHeight;
  //Ϊλͼ���ݷ����ڴ�
  hdib = GlobalAlloc(GHND,
                     dwbmbitssize + dwpalettesize + sizeof(BITMAPINFOHEADER));
  lpbi = (LPBITMAPINFOHEADER)GlobalLock(hdib);
  *lpbi = bi;

  // �����ɫ��
  hpal = GetStockObject(DEFAULT_PALETTE);
  if (hpal) {
    hdc = ::GetDC(NULL);
    holdpal = SelectPalette(hdc, (HPALETTE)hpal, false);
    RealizePalette(hdc);
  }

  // ��ȡ�õ�ɫ�����µ�����ֵ
  GetDIBits(hdc, hBitmap, 0, (UINT)bitmap.bmHeight,
            (LPSTR)lpbi + sizeof(BITMAPINFOHEADER) + dwpalettesize,
            (BITMAPINFO *)lpbi, DIB_RGB_COLORS);

  //�ָ���ɫ��
  if (holdpal) {
    SelectPalette(hdc, (HPALETTE)holdpal, true);
    RealizePalette(hdc);
    ::ReleaseDC(NULL, hdc);
  }

  //����λͼ�ļ�
  fh = CreateFile(szfilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  if (fh == INVALID_HANDLE_VALUE)
    return FALSE;

  // ����λͼ�ļ�ͷ
  bmfhdr.bfType = 0x4d42; // "bm"
  dwdibsize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) +
              dwpalettesize + dwbmbitssize;
  bmfhdr.bfSize = dwdibsize;
  bmfhdr.bfReserved1 = 0;
  bmfhdr.bfReserved2 = 0;
  bmfhdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) +
                     (DWORD)sizeof(BITMAPINFOHEADER) + dwpalettesize;

  //д��λͼ�ļ�ͷ
  WriteFile(fh, (LPSTR)&bmfhdr, sizeof(BITMAPFILEHEADER), &dwwritten, NULL);

  //д��λͼ�ļ���������
  WriteFile(fh, (LPSTR)lpbi, dwdibsize, &dwwritten, NULL);
  //���
  GlobalUnlock(hdib);
  GlobalFree(hdib);
  CloseHandle(fh);
  return TRUE;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "init"),
              Napi::Function::New(env, InitDll));
  return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)