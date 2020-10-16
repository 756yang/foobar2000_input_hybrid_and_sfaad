# foobar2000_input_hybrid_and_sfaad
 
# 这是foo_input_hybrid和foo_input_sfaad插件的源代码。
# 这是为平衡有损和无损音乐而创建的解码器包装。
# 有损音乐听感接近于无损而有着更小的文件大小，从频谱上来看可以几无差别以致人无法区分。
# 而无损音乐可以保存更好的音乐信息，音质更佳，但文件大小巨大。有些时候，我们既需要无损音乐也需要有损音乐，有损音乐适合移动设备的播放，无损音乐适合台式设备播放，同时存储有损和无损音乐是十分需要的。
# 我们发现无损音频去掉其有损部分後可以压缩得更小，这与直接压缩相比所消耗的磁盘空间基本一致，大约多出5%，而有损音乐就占约30%。
# 通过用两种压缩方式可以节省大量的磁盘空间，而本组件就是为了在附加无损音频存在的情况下，通过外包解码的方式还原原始音频。


# foo_input_hybrid and foo_input_sfaad source code.

# This is a decoder wrapper created to balance lossy and lossless music. Lossy music is close to lossless and has a smaller file size, so it is almost indistinguishable from the spectrum. Lossless music can save better music information, better sound quality, but the file size is huge. Sometimes, we need both lossless and lossy music. Lossy music is suitable for playing on mobile devices, and lossless music is suitable for playing on desktop devices. It is very necessary to store lossy and lossless music at the same time. We find that lossless audio can be compressed smaller after removing the lossy part. Compared with direct compression, lossless audio consumes about 5% more disk space, and lossy music accounts for about 30%. By using two compression methods can save a lot of disk space, and this component is to restore the original audio through the decoder wrapper whit additional lossless audio.