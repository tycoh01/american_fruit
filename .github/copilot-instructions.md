# Copilot Instructions

## 專案指導方針
- 網路爬蟲不可使用 SOAP；優先使用 libcurl Multi Interface（curl_multi_init）在單一執行緒中處理並行請求，並採用 Consumer-Producer 模型。