from setuptools import find_packages, setup

package_name = '1_pubsub'

setup(
    name=package_name,
    version='0.0.0',
    packages=find_packages(exclude=['test']),
    data_files=[
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        ('share/' + package_name, ['package.xml']),
        ('share/' + package_name + '/launch', ['launch/my_location.launch.py'])
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='edu56',
    maintainer_email='hoonie8530@gachon.ac.kr',
    description='TODO: Package description',
    license='TODO: License declaration',
    extras_require={
        'test': [
            'pytest',
        ],
    },
    entry_points={
        'console_scripts': [
            'publisher = 1_pubsub.publisher:main',  
            'subscriber = 1_pubsub.subscriber:main',  
            'processor = 1_pubsub.processor:main'
        ],
    },
)
