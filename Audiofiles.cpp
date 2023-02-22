#include "pch.h"
#include "Audiofiles.h"

/// <summary>
/// 指定したフォルダにあるファイルの一覧を返す
/// </summary>
/// <param name="folderPath">フォルダのパス</param>
/// <param name="file_names">ファイルの一覧を格納する配列</param>
/// <returns>成功すればtrue</returns>
bool getFileNames(std::string folderPath, std::vector<std::string>& file_names)
{
    using namespace std::filesystem;
    directory_iterator iter(folderPath), end;
    std::error_code err;

    for (; iter != end && !err; iter.increment(err)) {
        const directory_entry entry = *iter;

        file_names.push_back(entry.path().string());
    }

    /* エラー処理 */
    if (err) {
        std::cout << err.value() << std::endl;
        std::cout << err.message() << std::endl;
        return false;
    }
    return true;
}