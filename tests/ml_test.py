import numpy as np
from sklearn.model_selection import train_test_split
from sklearn.linear_model import LinearRegression
from sklearn.metrics import mean_squared_error

# 生成示例数据
X = np.random.rand(100, 1)  # 100 个样本，每个样本有 1 个特征
y = 2 * X.squeeze() + 1 + np.random.randn(100)  # 目标值是特征的 2 倍加上 1，再加上一些噪声

# 切分数据集为训练集和测试集
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# 创建线性回归模型
model = LinearRegression()

# 训练模型
model.fit(X_train, y_train)

# 进行预测
y_pred = model.predict(X_test)

# 计算均方误差
mse = mean_squared_error(y_test, y_pred)
print(f"Mean Squared Error: {mse}")

# 打印模型的系数和截距
print(f"Coefficient: {model.coef_[0]}")
print(f"Intercept: {model.intercept_}")
