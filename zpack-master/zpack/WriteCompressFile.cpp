#include "zpack.h"
#include "WriteCompressFile.h"
#include "zlib.h"

namespace zp
{

///////////////////////////////////////////////////////////////////////////////////////////////////
//also needed by hasher, make it global
//把srcFile的内容写入dstFile的offset位置
u32 writeCompressFile(FILE* dstFile, u64 offset, FILE* srcFile, u32 srcFileSize, u32 chunkSize, u32& flag,
						std::vector<u8>& chunkData,	std::vector<u8>& compressBuffer, std::vector<u32>& chunkPosBuffer)
{
	fseek(dstFile, offset, SEEK_SET);

	u32 chunkCount = (srcFileSize + chunkSize - 1) / chunkSize;		//分块处理
	chunkPosBuffer.resize(chunkCount);

	u32 packSize = 0;
	if (chunkCount > 1)
	{
		chunkPosBuffer[0] = chunkCount * sizeof(u32);							//第一位为块的数量 ，第二位为之前块读的大小+前一位的值
		fwrite(&chunkPosBuffer[0], chunkCount * sizeof(u32), 1, dstFile);
	}

	//BEGIN_PERF("compress");

	u8* dstBuffer = &compressBuffer[0];
	for (u32 i = 0; i < chunkCount; ++i)
	{
		u32 curChunkSize = chunkSize;

		//最后一块
		if (i == chunkCount - 1 && srcFileSize % chunkSize != 0)
		{
			curChunkSize = srcFileSize % chunkSize;
		}
		fread(&chunkData[0], curChunkSize, 1, srcFile);

		u32 dstSize = chunkSize;

		//压缩，写入
		int ret = compress(dstBuffer, &dstSize, &chunkData[0], curChunkSize);

		if (ret != Z_OK	|| dstSize >= curChunkSize)
		{
			//compress failed or compressed size greater than origin, write raw data
			fwrite(&chunkData[0], curChunkSize, 1, dstFile);
			dstSize = curChunkSize;
		}
		else
		{
			fwrite(dstBuffer, dstSize, 1, dstFile);
		}


		if (i + 1 < chunkCount)
		{
			chunkPosBuffer[i + 1] = chunkPosBuffer[i] + dstSize;
		}
		packSize += dstSize;
	}

	//END_PERF

	if (chunkCount > 1)
	{
		packSize += chunkCount * sizeof(u32);			//包大小 = 压缩完包大小+块数量*4
		fseek(dstFile, offset, SEEK_SET);
		fwrite(&chunkPosBuffer[0], chunkCount * sizeof(u32), 1, dstFile);
	}
	else if (packSize == srcFileSize)
	{
		//only 1 chunk and not compressed, entire file should not be compressed
		flag &= (~FILE_COMPRESS);
	}
	return packSize;
}

}
