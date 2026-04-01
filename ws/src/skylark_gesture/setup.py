from setuptools import find_packages, setup

package_name = 'skylark_gesture'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
    ],
    install_requires=['setuptools','onnxruntime','numpy'],
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
            'gesture_node = skylark_gesture.gesture_node:main'
        ],
    },
)
