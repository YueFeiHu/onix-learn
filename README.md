# onix-learn

来源于 https://github.com/StevenBaby/onix

# 环境

`Ubuntu20.04 (使用清华源)`

`nasm (sudo apt install nasm)`

`bochs (sudo apt install bochs)`


## bochs使用注意事项
在使用bochs创建镜像时，出现了一些错误，原因是bochs版本和原作者不一样，新的做法是`bximage -q` 然后选择 create, hd, flat, 512, 16 , master.img，其实就是原命令的互动选择。

原作者使用的是archlinux，我用的和他不一样，在使用bochs出现一些问题，通过搜索引擎解决了，使用了如下几条命令：

`sudo apt-get install bochs-x`

`sudo apt-get install -y bochsbios`

`sudo apt-get install -y vgabios`

