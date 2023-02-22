#include "pch.h"
#include "Audiofiles.h"

/// <summary>
/// �w�肵���t�H���_�ɂ���t�@�C���̈ꗗ��Ԃ�
/// </summary>
/// <param name="folderPath">�t�H���_�̃p�X</param>
/// <param name="file_names">�t�@�C���̈ꗗ���i�[����z��</param>
/// <returns>���������true</returns>
bool getFileNames(std::string folderPath, std::vector<std::string>& file_names)
{
    using namespace std::filesystem;
    directory_iterator iter(folderPath), end;
    std::error_code err;

    for (; iter != end && !err; iter.increment(err)) {
        const directory_entry entry = *iter;

        file_names.push_back(entry.path().string());
    }

    /* �G���[���� */
    if (err) {
        std::cout << err.value() << std::endl;
        std::cout << err.message() << std::endl;
        return false;
    }
    return true;
}