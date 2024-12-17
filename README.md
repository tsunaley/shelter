# 为高敏感性机密计算工作负载设计构建一个安全且高效的机密容器平台

##### 作品完成和提交方式：选择本赛题的参赛队伍需要首先复刻（Fork）本项目，然后在复刻的项目中添加参赛队员、合作完成作品开发即可，无需提交PR到赛题项目。如果作品为文档形式，也请将作品文档提交到项目代码库中。在作品完成过程中，围绕作品的相关讨论等可以以疑修（Issue）形式发布和讨论，也可使用里程碑对整个任务进行规划管理。

### 1. 赛题说明
随着云原生的快速发展，机密数据的安全防护变得至关重要，涉密程序的运行需要更高要求的安全保护，为此可信执行环境应运而生。题目要求TEE平台上设计一个高效且安全的机密容器平台，该平台应尽量减小TCB，增加安全性，为高敏感的后台服务型工作负载提供通用的容器运行平台，例如权限认证服务，密钥管理服务，隐私数据库等。

### 2. 赛题要求
1. 设计的容器平台应具备通用性，且保证运行的工作负载得到不低于TEE平台的安全防护能力；
2. 应在权衡性能的前提下，尽量减小TCB，提升安全性；
3. 至少支持SGX平台，可选支持TDX等机密虚拟机形态的TEE平台。

### 3. 赛题导师
zhang.jia@linux.alibaba.com

### 4. 参考资料
[1] 龙蜥社区机密计算SIG：

### 5. 编译
编译工具前请确保已安装一下依赖项：
```
sudo apt install -y pkg-config libssl-dev asciidoctor
```

#### 编译工具
执行以下命令，编译构建和运行项目所需的工具：

```
make tools
```

#### 编译项目
运行以下命令编译 Shelter 项目：

```
make
```
### 6. 使用方法
编译完成后，可以使用 Shelter 命令行工具来构建机密虚拟机并运行敏感程序。
#### Shelter 命令
```
./shelter help
```

### 7. 测试示例

#### 将敏感程序放入机密虚拟机中
将要放入敏感程序机密虚拟机中的敏感程序静态编译，可以使用`tests/ `目录下的`md5.c`测试。
静态编译：
```
cd tests/
gcc -static -o md5 md5.c
```

构建Shelter镜像并指定敏感程序，敏感程序会放入机密虚拟机的`/bin`目录下：
```
cd ../
sudo ./shelter build -t md5 -f tests/md5 -a "/usr/local/bin/attestation-agent"

```

客户端运行远程认证：
```
cd ra/
sh ra.sh
```

运行敏感程序：
```
cd ../
sudo ./shelter run -t md5 -c /bin/md5 -i <Your remote attestation IP address> -m <Memory size(2G)>
```

#### 将根文件系统放入虚拟机中
将要放入敏感程序机密虚拟机中的敏感程序静态编译，可以使用`tests/ `目录下的`Dockerfile`测试。
构建镜像运行python程序：
```
cd tests/
docker build -t my-ml-app .
```

运行docker容器：
```
docker run my-ml-app
```

导出rootfs：
```
sudo docker export <Container id> -o rootfs.tar
```

向shelter指定要使用的根文件系统：
```
cd ../
sudo ./shelter build -t ml -r tests/rootfs.tar
```

客户端运行远程认证：
```
cd ra/
sh ra.sh
```

运行根文件系统中的程序：
```
cd ../
sudo ./shelter run -t ml -c "/usr/local/bin/python /app/ml_test.py" -m <Memory size(4G)> -i <Your remote attestation IP address>
```
### 8. 说明文档与演示视频
相关的说明文档与演示视频在`doc/`目录下。  
项目功能说明书：[doc/操作系统开源创新大赛项目功能说明书.pdf](https://www.gitlink.org.cn/oT3lu6drj2/aqqgxdjmrqpt/tree/master/doc%2F%E6%93%8D%E4%BD%9C%E7%B3%BB%E7%BB%9F%E5%BC%80%E6%BA%90%E5%88%9B%E6%96%B0%E5%A4%A7%E8%B5%9B%E9%A1%B9%E7%9B%AE%E5%8A%9F%E8%83%BD%E8%AF%B4%E6%98%8E%E4%B9%A6.pdf)  
操作系统开源创新大赛原创承诺书: [doc/操作系统开源创新大赛原创承诺书.pdf](https://www.gitlink.org.cn/oT3lu6drj2/aqqgxdjmrqpt/tree/master/doc%2F%E6%93%8D%E4%BD%9C%E7%B3%BB%E7%BB%9F%E5%BC%80%E6%BA%90%E5%88%9B%E6%96%B0%E5%A4%A7%E8%B5%9B%E5%8E%9F%E5%88%9B%E6%89%BF%E8%AF%BA%E4%B9%A6.pdf)  
演示视频：[doc/演示视频.mp4](https://www.gitlink.org.cn/oT3lu6drj2/aqqgxdjmrqpt/tree/master/doc%2F%E6%BC%94%E7%A4%BA%E8%A7%86%E9%A2%91.mp4)  
项目PPT：[doc/一斤鸭梨-应用创新赛道-赛题8：为高敏感型机密计算工作负载设计构建一个安全且高效的机密容器平台.pptx](https://www.gitlink.org.cn/oT3lu6drj2/aqqgxdjmrqpt/tree/master/doc%2F%E4%B8%80%E6%96%A4%E9%B8%AD%E6%A2%A8-%E5%BA%94%E7%94%A8%E5%88%9B%E6%96%B0%E8%B5%9B%E9%81%93-%E8%B5%9B%E9%A2%988%EF%BC%9A%E4%B8%BA%E9%AB%98%E6%95%8F%E6%84%9F%E5%9E%8B%E6%9C%BA%E5%AF%86%E8%AE%A1%E7%AE%97%E5%B7%A5%E4%BD%9C%E8%B4%9F%E8%BD%BD%E8%AE%BE%E8%AE%A1%E6%9E%84%E5%BB%BA%E4%B8%80%E4%B8%AA%E5%AE%89%E5%85%A8%E4%B8%94%E9%AB%98%E6%95%88%E7%9A%84%E6%9C%BA%E5%AF%86%E5%AE%B9%E5%99%A8%E5%B9%B3%E5%8F%B0.pptx)
