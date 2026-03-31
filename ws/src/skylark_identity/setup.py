from setuptools import find_packages, setup

package_name = 'skylark_identity'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    package_data={
        package_name: ['static/*'],
    },
    install_requires=['setuptools', 'insightface', 'onnxruntime', 'opencv-python', 'numpy','fastapi','uvicron','python-multipart'],
    zip_safe=True,
    maintainer='akhi',
    maintainer_email='akhi@todo.todo',
    description='TODO: Package description',
    license='Apache-2.0',
    extras_require={
        'test': [
            'pytest',
        ],
    },
    entry_points={
        'console_scripts': [
            'identity_node = skylark_identity.identity_node:main'
        ],
    },
)
