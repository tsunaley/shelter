# 使用 Python 官方镜像作为基础镜像
FROM python:3.11-slim

# 设置工作目录
WORKDIR /app

# 复制本地代码到容器
COPY . /app

# 安装必要的 Python 包
RUN pip install numpy scikit-learn

# 运行 Python 代码
CMD ["python", "ml_test.py"]
