#pragma once

#include <curl/curl.h>


#include <cstdio>
#include <cstddef>
#include <cstdint>

#include <memory>
#include <functional>

#include <array>
#include <string>
#include <string_view>
#include <fstream>
#include <filesystem>
#include <exception>
#include <stdexcept>
#include <limits>
#include <system_error>
#include <optional>

#include <format>

 namespace adecc::net {

 struct DownloadProgress {
	curl_off_t                iDownloadedBytes;
	std::optional<curl_off_t> iTotalBytes;
	};


 class CurlDownloader final {
	public:
	   using ProgressCallback = std::function<bool (DownloadProgress const&)>;

	private:
	   class CurlEnvironment final {
		  public:
			 CurlEnvironment() {
				if(CURLcode const aResult = curl_global_init(CURL_GLOBAL_DEFAULT); aResult != CURLE_OK) {
				   throw std::runtime_error { std::format("curl_global_init failed: {}",
														   curl_easy_strerror(aResult)	  )  };
				   }
				}

			 ~CurlEnvironment() {
				curl_global_cleanup();
				}

			 CurlEnvironment(CurlEnvironment&&) noexcept = delete;
			 CurlEnvironment& operator = (CurlEnvironment&&) noexcept = delete;
		  };

	   class TemporaryFile final {
		  private:
			 std::filesystem::path aPath;
			 bool                  boInstalled = false;

		  public:
			 explicit TemporaryFile(std::filesystem::path const& aTmpPath) : aPath { aTmpPath } {
				}

			 ~TemporaryFile() noexcept {
				if(boInstalled) {
				   std::error_code aError;
				   std::filesystem::remove(aPath, aError);
				   }
				}

			 TemporaryFile(TemporaryFile&&) noexcept = delete;
			 TemporaryFile& operator = (TemporaryFile&&) noexcept = delete;

			 void Release() noexcept {
				boInstalled = true;
				}
		  };

	   struct TransferContext {
		  std::ofstream           aFile;
		  ProgressCallback const* pProgress  = nullptr;
		  std::exception_ptr      aCallbackError;
		  bool                    boCancelled = false;
		  };


	   CurlEnvironment aEnvironment;
	   std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> aCurl;

	   static std::size_t WriteCallback(char* pData, std::size_t uSize, std::size_t uCount, void* pUser) noexcept {
		  // pUser != nullptr per definitiuon ?
		  TransferContext& aContext = *static_cast<TransferContext*>(pUser);
		  try {
			 if(uSize != 0 && uCount > std::numeric_limits<std::size_t>::max())
				throw std::runtime_error { "HTTP write exceeds size" };

			 std::size_t uBytes = uSize * uCount;
			 if(uBytes > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max()))
					throw std::runtime_error { "HTTP write exceeds streamsize" };

			 aContext.aFile.write(pData, static_cast<std::streamsize>(uBytes));
			 // settings for stream with exception or check retval

			 return uBytes;
			 }
		  catch (...) {
			 aContext.aCallbackError = std::current_exception();
			 return 0;
			 }
		  }

	   static int ProgressCallbackBridge(void* pUser, curl_off_t uTotal, curl_off_t uCurrent,
										 curl_off_t, curl_off_t) {
		  TransferContext& aContext = *static_cast<TransferContext*>(pUser);
		  try {
			 DownloadProgress aProgress;
			 aProgress.iDownloadedBytes = uCurrent;
			 aProgress.iTotalBytes      = uTotal;

			 if(aContext.pProgress  != nullptr) {
				if(!(*aContext.pProgress)(aProgress)) {
				   aContext.boCancelled = true;
				   return 1;
				   }
				}
			 return 0;
			 }
		  catch(...) {
			 aContext.aCallbackError = std::current_exception();
			 return 1;
			 }

		  }

	   void DownloadFileImpl(std::string_view svUrl, std::filesystem::path const& aPath,
							 ProgressCallback const* pProgress) {

		  if(svUrl.empty() || aPath.empty() || !aPath.has_filename() )
			 throw std::invalid_argument { "URL and out name are required" };

		  std::filesystem::path aTmpPath = aPath;
		  aTmpPath += ".part";

		  TemporaryFile aTmpFile(aTmpPath);

		  {
		  TransferContext aContext { };
		  aContext.aFile.open(aTmpPath, std::ios::binary | std::ios::trunc);
		  if(!aContext.aFile) {
			 throw std::runtime_error { std::format("Can't open File {}", aTmpPath.string()) };
			 }

		  aContext.aFile.exceptions(std::ios::badbit | std::ios::failbit);
		  aContext.pProgress = pProgress;

		  std::array<char, CURL_ERROR_SIZE> aErrorBuffer {};
		  std::string const strUrl = { svUrl.data(), svUrl.size() };

		  curl_easy_reset(aCurl.get());

		  auto SetOption = [this](CURLoption opt, auto aValue) {
			 if(CURLcode ret = curl_easy_setopt(aCurl.get(), opt, aValue); ret != CURLE_OK) {
				throw std::runtime_error { std::format("curl_easy_setopt({}) failed: {}.",
										   static_cast<long>(opt), curl_easy_strerror(ret)) };
				}
			 };

		  SetOption(CURLOPT_ERRORBUFFER, aErrorBuffer.data());

		  // we were here ;-)

		  SetOption(CURLOPT_URL,                 strUrl.c_str());
		  SetOption(CURLOPT_PROTOCOLS_STR,       "http,https");
		  SetOption(CURLOPT_REDIR_PROTOCOLS_STR, "http,https");
		  SetOption(CURLOPT_WRITEFUNCTION,       &CurlDownloader::WriteCallback);
		  SetOption(CURLOPT_WRITEDATA,           &aContext);
		  SetOption(CURLOPT_FOLLOWLOCATION,      1L);
		  SetOption(CURLOPT_MAXREDIRS,           5L);
		  SetOption(CURLOPT_FAILONERROR,         1L);
		  SetOption(CURLOPT_CONNECTTIMEOUT,     30L);
		  SetOption(CURLOPT_TIMEOUT,             0L);
		  SetOption(CURLOPT_LOW_SPEED_LIMIT,     1L);
		  SetOption(CURLOPT_LOW_SPEED_TIME,    120L);
		  SetOption(CURLOPT_NOSIGNAL,            1L);
		  SetOption(CURLOPT_SSL_VERIFYPEER,      1L);
		  SetOption(CURLOPT_SSL_VERIFYHOST,      2L);

          #if defined(_WIN32)
			 SetOption(CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
		  #endif


		  if (pProgress != nullptr) {
			 SetOption(CURLOPT_XFERINFOFUNCTION, &CurlDownloader::ProgressCallbackBridge);
			 SetOption(CURLOPT_XFERINFODATA, &aContext);
			 SetOption(CURLOPT_NOPROGRESS, 0L);
			 }
		  else {
			 SetOption(CURLOPT_NOPROGRESS, 1L);
			 }

		  CURLcode const eResult = curl_easy_perform(aCurl.get());

		  if (aContext.aCallbackError) {
			 std::rethrow_exception(aContext.aCallbackError);
			 }

		  if (aContext.boCancelled) {
			 throw std::runtime_error("Download cancelled by progress callback");
			 }

		  if (eResult != CURLE_OK) {
			 throw std::runtime_error { std::format("Download failed: {}",
										aErrorBuffer[0] != '\0' ? aErrorBuffer.data()
																: curl_easy_strerror(eResult)) };
			 }

		  long lHttpStatus = 0;
		  CURLcode const eInfo = curl_easy_getinfo(aCurl.get(), CURLINFO_RESPONSE_CODE, &lHttpStatus);

		  if (eInfo != CURLE_OK) {
			 throw std::runtime_error { std::format("curl_easy_getinfo failed: {}",
														curl_easy_strerror(eInfo)) };
			 }

		  if (lHttpStatus != 200L) {
			 throw std::runtime_error { std::format("Unexpected HTTP response: {}", lHttpStatus) };
			 }

		  aContext.aFile.flush();
		  aContext.aFile.close();
		  } // end of scope

		  std::filesystem::rename(aTmpPath, aPath);
		  aTmpFile.Release();
		  }

	public:

	   CurlDownloader() : aEnvironment(),  aCurl(curl_easy_init(), &curl_easy_cleanup) {
		  if (!aCurl) {
			 throw std::runtime_error("curl_easy_init failed");
			 }
		  }

	   ~CurlDownloader() noexcept = default;

	   CurlDownloader(CurlDownloader const&) = delete;
	   CurlDownloader& operator=(CurlDownloader const&) = delete;
	   CurlDownloader(CurlDownloader&&) noexcept = delete;
	   CurlDownloader& operator=(CurlDownloader&&) noexcept = delete;

	   void DownloadFile(std::string_view svUrl, std::filesystem::path const& aPath) {
		  DownloadFileImpl(svUrl, aPath, nullptr);
		  }

	   void DownloadFile(std::string_view svUrl, std::filesystem::path const& aPath,
						 ProgressCallback const& fnProgress) {

		  if (!fnProgress) {
			 throw std::invalid_argument("Progress callback must not be empty");
			 }

		  DownloadFileImpl(svUrl, aPath, &fnProgress);
		  }

 };

 }

