import argparse
from selenium.webdriver import Chrome, ChromeOptions
from selenium.webdriver.common.by import By
import sys
import pprint
import os
import time
import socketserver
import threading
import http.server
import socket

# This script hosts the browser based flap generator on a local server, 
# opens the index.html file in a headless Chrome browser, 
# and downloads the generated flap zip file.

class TestHTTPServer(socketserver.TCPServer):
    allow_reuse_address = True

    def server_bind(self):
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        super().server_bind()

    def shutdown(self):
        self.socket.close()
        super().shutdown()

def test(index_file_path, download_dir):
    # Serve the directory containing index.html
    os.chdir(os.path.dirname(index_file_path))
    Handler = http.server.SimpleHTTPRequestHandler
    httpd = TestHTTPServer(("", 8000), Handler)

    def start_server():
        with httpd:
            httpd.serve_forever()
    
    server_thread = threading.Thread(target=start_server)
    server_thread.daemon = True
    server_thread.start()

    # Set up the ChromeOptions
    options = ChromeOptions()
    options.add_argument('--headless')
    options.add_argument('--disable-gpu')
    options.add_argument('--no-sandbox')
    options.add_argument('--disable-dev-shm-usage')
    prefs = {
    "download.default_directory": download_dir,
    "download.prompt_for_download": False,
    "download.directory_upgrade": True,
    "safebrowsing.enabled": True
    }
    options.add_experimental_option("prefs", prefs)

    # Initialize the WebDriver
    driver = Chrome(options=options)

    # Open the index.html file
    local_url = "http://localhost:8000/index.html"
    driver.get(local_url)
    time.sleep(1)

    # Execute JavaScript to call the function
    driver.execute_script("exportSvgForGerbolyzation();")

    # Wait for the download to complete
    time.sleep(1)

    # Close the WebDriver
    driver.quit()

    # Properly shutdown the server
    httpd.shutdown()
    httpd.server_close()
    server_thread.join()

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Run the browser-based flap generator and download the generated flap zip file.')
    parser.add_argument('index_file_path', type=str, help='Path to the index.html file')
    parser.add_argument('download_dir', type=str, help='Directory to save the downloaded file')

    args = parser.parse_args()

    test(args.index_file_path, args.download_dir)