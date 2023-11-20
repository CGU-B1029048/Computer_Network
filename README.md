This is the repo for Computer Network homework and projects.
2023 Computer Network Course

# Term Project (Part 1)
使用 C++撰寫可下載網站供離線瀏覽的程式,功能需求如下:
## 基本功能
1. 提供兩種終端機模式供使用者輸入參數 (URL 與輸出目錄)
    + 命令列模式:使用者於執行時直接將必要的參數透過命令列輸入。
    + 互動模式:程式執行後會輸出提示訊息要求使用者輸入參數。
2. 基本功能#1 中必須檢查使用者輸入的參數是否有誤,並且輸出適當的說明訊息。例如:
    + 使用者輸入不正確的 URL 格式
    + 使用者輸入的輸出目錄無法寫入
3. 僅需要下載使用者所輸入之 URL 所代表之物件以及其包含的其他物件即可,不需要處理向下展開的遞迴下載,但必須視需要修改物件的位址以供離線瀏覽。
4. 下載結束後顯示狀態 (URL 是否有效) 與統計資訊 (如檔案數量、下載總容量、下載時間
等)。
## 進階功能
1. 下載過程顯示各物件的下載狀況 (物件資訊、進度、速度、預計剩餘完成時間)。
2. 可支援 persistent 與 non-persistent (預設方式) connections。

# 評分方式:
## 程式 (80 points)
1. 具有題目所要求的基本功能 (55 points)
2. 具有題目所要求的進階功能 (15 points)
3. 程式可讀性, 如註解, 縮排, 變數命名等 (5 points)
4. 使用者介面與美觀 (5 points)
## 作業報告 (20 points)
1. 內容是否充實 (15 points)
2. 整齊美觀 (5 points)

# 作業報告至少需包含下列內容:
1. 封面
2. 操作說明: 說明程式的操作方式
3. 程式說明: 說明程式的架構與功能, 若有使用特別的技巧也請說明
4. 參考資料: 若有參考教科書以外的書籍或網路上的資料, 請在此部份詳列

# 注意事項:
1. 程式檔案中請包含已編譯好的執行檔案。
2. 必須自行撰寫程式產生 HTTP requests 訊息與解析 HTTP responses,不可使用程式語言提供的 HTTP 相關函式庫。
3. 程式僅可實作題目要求的功能,若包含題目未要求功能的程式片段,將視程度予以扣分,嚴重者可能被扣至零分。
4. 此作業必須在終端機模式下執行,不可提供圖形化介面。
5. 若有參考或是使用他人所撰寫的部份程式片段,請務必於報告中的參考資料部份詳列,否
則視為抄襲,此次作業分數為零分。
6. 參考他人程式請斟酌程度,勿直接複製貼上,否則仍會被扣分。情況嚴重者,將視為抄襲。

# 測試[網頁](http://hsccl.us.to/index.htm)
以下網頁係供測試使用,並不代表程式只需要能處理此網頁,請另外尋找其他網頁進行測試,以確定程式可滿足題目要求 