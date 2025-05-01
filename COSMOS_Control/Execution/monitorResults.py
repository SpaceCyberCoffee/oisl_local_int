import os
import time
import Utilities.PlotGeneration as pgen
import matplotlib.pyplot as plt
import io
import base64
import Utilities.ResultsEmailGeneration
import Utilities.FileHandling
import smtplib
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from email.mime.image import MIMEImage
from datetime import datetime

def check_for_new_files(directory):

    known_files = set(os.listdir(directory))

    current_files = set(os.listdir(directory))
    new_files = current_files - known_files
    if new_files:
        print(f"Neue Datei(en) gefunden: {new_files}")
        # Hier können Sie Funktionen ausführen, die mit den neuen Dateien arbeiten.

    return current_files


def sendEmailWithPlot(directory_to_watch='/mnt/extras/SSD/NOS3_RBT/nos3_uli/nos3/COSMOS_Control/Execution/tm_data', folderForAnalysisOfTM='/mnt/extras/SSD/NOS3_RBT/TM_test_data', interval=600):

    fileNames = ['TIME_DATA', 'CAPACITY_DATA', 'VOLTAGE_DATA', 'SOC_DATA', 'C_DATA'] #, 'SUBSECONDS_DATA']

    old_files = []

    while True:
        current_datetime = datetime.now()
        date_str = current_datetime.strftime('%Y-%m-%d %H:%M:%S')
        print(f"Entered loop: {date_str}")
        time.sleep(interval)

        current_files = sorted(os.listdir(directory_to_watch), key=str.lower)

        newFiles = [item for item in current_files if item not in old_files]
        Utilities.FileHandling.copyFilesToFolder(directory_to_watch, newFiles, folderForAnalysisOfTM)

        if current_files != old_files:
            timeVals = []
            for fi in current_files:
                if 'TIME_DATA' in fi:
                    print(fi)
                    with open((directory_to_watch + '/' + fi), 'r') as file:
                        timeVals = timeVals + [float(line.strip().split()[0]) for line in file]

            valuesToDisplay = {}
            titles = {}
            cnt = 0
            for fiNa in fileNames:
                if fiNa != 'TIME_DATA':
                    valuesToDisplay[cnt] = []
                    titles[cnt] = fiNa
                    for fi in current_files:
                        if fiNa in fi:
                            print(fi)
                            #fileVals = []
                            with open((directory_to_watch + '/' + fi), 'r') as file:
                                for line in file:
                                    #fiNaValues = fiNaValues + [line.strip().split()[0] for line in file]
                                    valuesToDisplay[cnt].append(float(line.strip().split()[0]))
                                #fileVals = [float(line.strip().split()[0]) for line in file]
                    cnt = cnt + 1

            #figures = [plt.figure() for _ in range(len(valuesToDisplay))]  # Example for 3 figures
            encoded_images = []
            for i in range(len(valuesToDisplay)): #enumerate(figures):

                plt.figure()
                plt.title(titles[i])
                if len(timeVals) <= len(valuesToDisplay[i]):
                    plt.plot(timeVals, valuesToDisplay[i][0:len(timeVals)])
                else:
                    plt.plot(timeVals[0:len(valuesToDisplay[i])], valuesToDisplay[i])
                buf = io.BytesIO()
                plt.savefig(buf, format='png')
                buf.seek(0)
                encoded_images.append(buf.getvalue())
                #encoded_images.append(base64.b64encode(buf.read()).decode('utf-8'))
                plt.close()
                '''ax = fig.add_subplot(111)
                ax.set_ylim([0, max(valuesToDisplay[i]) + 1])
                #ax.plot(list(range(len(valuesToDisplay[i]))), valuesToDisplay[i])
                if len(timeVals) <= len(valuesToDisplay[i]):
                    ax.plot(timeVals, valuesToDisplay[i][0:len(timeVals)])
                else:
                    ax.plot(timeVals[0:len(valuesToDisplay[i])], valuesToDisplay[i])
                ax.set_title(f'{titles[i]}')
                #plt.show()
                #for entry in valuesToDisplay[i]:
                    #print(entry)
                buf = io.BytesIO()
                plt.savefig(buf, format='png')
                buf.seek(0)
                encoded_images.append(base64.b64encode(buf.read()).decode('utf-8'))
                plt.close()'''

            '''encoded_images = []
            for fig in figures:
                buf = io.BytesIO()
                plt.savefig(buf, format='png')
                buf.seek(0)
                encoded_images.append(base64.b64encode(buf.read()).decode('utf-8'))'''


            msg = MIMEMultipart('related')
            msg['Subject'] = 'NOS3 simulation results' + ' (python-automated-Email)'
            msg['From'] = 'ulrich.kling@dlr.de'
            msg['To'] = 'ulrich.kling@dlr.de'

            html = '<html><body>'
            for i, img_data in enumerate(encoded_images):
                html += f'<img src="cid:image{i}"><br>'
            html += '</body></html>'
            msg_html = MIMEText(html, 'html')

            msg.attach(msg_html)

            for i, img_data in enumerate(encoded_images):
                msg_img = MIMEImage(img_data, 'png')
                msg_img.add_header('Content-ID', f'<image{i}>')
                msg.attach(msg_img)

            '''html_content = '<html><head></head><body>'
            for i, img_data in enumerate(encoded_images):
                html_content += f'<h1>Figure {i + 1}:</h1><img src="cid:figure{i + 1}"><br>'
            html_content += '</body></html>'
            msg.attach(MIMEText(html_content, 'html'))

            # Step 4: Attach each figure to the email
            for i, img_data in enumerate(encoded_images):
                buf = io.BytesIO()
                #plt.savefig(buf, format='png')
                #buf.seek(0)
                image = MIMEImage(img_data, 'png')
                image.add_header('Content-ID', f'<figure{i + 1}>')
                msg.attach(image)

            # Step 5: Convert the message to a string
            email_string = msg.as_string()'''



            smtpObj = smtplib.SMTP('smtprelay.dlr.de')
            smtpObj.sendmail('ulrich.kling@dlr.de', 'ulrich.kling@dlr.de', msg.as_string())
            smtpObj.quit()
            print('Successfully sent email')


        if old_files != current_files:
            old_files = current_files

        #time.sleep(interval)

def sendEmail( message, senderEmail='ulrich.kling@dlr.de', receiverEmail='ulrich.kling@dlr.de'):
    try:
        smtpObj = smtplib.SMTP('smtprelay.dlr.de')
        smtpObj.sendmail(senderEmail, receiverEmail, message)
        smtpObj.quit()
        print('Successfully sent email')

    except smtplib.SMTPException:
        print('Error: unable to send email')


import os


def get_latest_folder_name(directory_path):
    try:
        # Get a list of all directories in the specified path
        all_folders = [f for f in os.listdir(directory_path) if os.path.isdir(os.path.join(directory_path, f))]

        # Sort the list of folders by creation time (most recent first)
        sorted_folders = sorted(all_folders, key=lambda f: os.path.getctime(os.path.join(directory_path, f)),
                                reverse=True)

        # Get the name of the latest created folder
        latest_folder_name = sorted_folders[0] if sorted_folders else None

        return latest_folder_name
    except Exception as e:
        return str(e)


if __name__ == '__main__':

    #currentTmDataFolder = 'tm_data_2024-04-17'
    dataFolder = '/mnt/extras/SSD/NOS3_RBT/nos3_uli/nos3/COSMOS_Control/Execution/tm_data/'
    currentTmDataFolder = get_latest_folder_name(dataFolder)
    print(currentTmDataFolder)
    sendEmailWithPlot(f'{dataFolder}{currentTmDataFolder}')
