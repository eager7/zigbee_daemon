from setuptools import setup, find_packages

setup(name='mLib',
    version='1.0',
    description='My Lib of use module',
    author='PCT',
    maintainer_email='panchangtao@gmail.com',
    license='BSD',
    include_package_data=True,
    zip_safe=False,
    packages=find_packages(),
    install_requires=[
        # 'pyzmq>1.0',
    ],
)

