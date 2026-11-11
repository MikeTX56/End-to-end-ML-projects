import numpy as np

class PCA:
    def __init__(self, n_components):
        self.n_components = n_components
        self.components = None
        self.mean = None

    def fit(self, X):
        # 1. Mean Centering
        self.mean = np.mean(X, axis=0)
        X = X - self.mean
        
        # 2. Covariance Matrix
        # np.cov expects features as rows, so we transpose X
        cov = np.cov(X.T)
        
        # 3. Eigen Decomposition
        # np.linalg.eig returns eigenvalues and eigenvectors
        eigenvalues, eigenvectors = np.linalg.eig(cov)
        
        # Eigenvectors are columns, transpose to make them rows
        eigenvectors = eigenvectors.T
        
        # 4. Sort Eigenvectors
        # Sort based on eigenvalues in descending order
        idxs = np.argsort(eigenvalues)[::-1]
        eigenvalues = eigenvalues[idxs]
        eigenvectors = eigenvectors[idxs]
        
        # 5. Store Components
        self.components = eigenvectors[0:self.n_components]

    def transform(self, X):
        # Project Data
        X = X - self.mean
        return np.dot(X, self.components.T)

# --- Usage Example ---
if __name__ == "__main__":
    from sklearn import datasets
    import matplotlib.pyplot as plt

    # Load sample data (Iris dataset)
    data = datasets.load_iris()
    X = data.data
    y = data.target

    # Project data to 2 principal components
    pca = PCA(2)
    pca.fit(X)
    X_projected = pca.transform(X)

    print("Original Shape:", X.shape)
    print("Transformed Shape:", X_projected.shape)

    # Plot results
    plt.scatter(X_projected[:, 0], X_projected[:, 1], c=y, edgecolor="none", alpha=0.8, cmap="viridis")
    plt.xlabel("Principal Component 1")
    plt.ylabel("Principal Component 2")
    plt.colorbar()
    plt.show()
