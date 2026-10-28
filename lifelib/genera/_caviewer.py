'''
Downloads CAViewer if not already downloaded.
'''

import os
import zipfile

DOWNLOAD_LINK = "https://gist.github.com/jedlimlx/a4ad9e4bddf1bc0fcfe220c4c150ff11/raw/02a656bc9dda3d6816d0bbcacb0c695b4032dc2c/CAViewer-Linux.zip"


def download():

    this_dir = os.path.dirname(os.path.abspath(__file__))
    zip_location = os.path.join(this_dir, 'CAViewer-Linux.zip')
    caviewer_path = this_dir + "/bin/CAViewer"

    try:
        from urllib import urlretrieve
    except ImportError:
        from urllib.request import urlretrieve

    if not os.path.exists(caviewer_path):  # Checking if CAViewer exists

        print("Downloading CAViewer...")
        urlretrieve(DOWNLOAD_LINK, zip_location)

        print("Unzipping...")
        with zipfile.ZipFile(zip_location, "r") as z:
            z.extractall(this_dir)

        print("Download complete!")

        os.chmod(caviewer_path, 0o755)
