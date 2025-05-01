import smtplib
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
import base64

# --------------------------------------------------------- | BEGIN class definition |
class ResultsEmailGeneration:

    def __init__(self, senderEmail, receiverEmail, subject, resultsDict={}, pathToFile=None, figurelist=[]):
        self.senderEmail = senderEmail
        self.receiverEmail = receiverEmail
        self.subject = subject + ' (python-automated-Email)'
        self.linkToFile = pathToFile

        emailMsg = ''
        if resultsDict != {}:
            self.resultsDict = resultsDict
            emailMsg = self._createHTMLEmail()
        elif figurelist != []:
            self.figureList = figurelist
            emailMsg = self._createEmailWithFigures()


        self._sendEmail(emailMsg.as_string())

    def _createHTMLEmail(self):
        # Create message container - the correct MIME type is multipart/alternative.
        msg = MIMEMultipart('alternative')
        msg['Subject'] = self.subject
        msg['From'] = self.senderEmail
        msg['To'] = self.receiverEmail

        # Create the body of the message (a plain-text and an HTML version).
        # text = "Hi!\nHow are you?\nHere is the link for activation:\nhttp://example2.com"
        html = """\
                <html>
                  <head></head>
                    <style>
                    table, th, td {
                      border:1px solid black;
                    }
                    </style>
                  <body>"""

        html = html + "".join(self._createHTMLTable())

        html = html + """            
                        </body>
                    </html>
                    """

        if self.linkToFile != None:
            html = html + """
                    <p> <a href=""" + """\"""" + self.linkToFile + """\"""" + """>""" + self.linkToFile + """</a> </p>
                    """

        # print(html)
        # Record the MIME types of both parts - text/plain and text/html.
        # part1 = MIMEText(text, 'plain')
        part2 = MIMEText(html, 'html')

        # Attach parts into message container.
        # msg.attach(part1)
        msg.attach(part2)

        return msg

    def _createEmailWithFigures(self):
        # Create message container - the correct MIME type is multipart/alternative.
        msg = MIMEMultipart('alternative')
        msg['Subject'] = self.subject
        msg['From'] = self.senderEmail
        msg['To'] = self.receiverEmail

        html_content = '<html><head></head><body>'
        for i, img_data in enumerate(self.figureList, 1):
            html_content += f'<h1>Figure {i}:</h1><img src="data:image/png;base64,{img_data}"><br>'
        html_content += '</body></html>'
        msg.add_alternative(html_content, subtype='html')

        # print(html)
        # Record the MIME types of both parts - text/plain and text/html.
        # part1 = MIMEText(text, 'plain')
        #part2 = MIMEText(html, 'html')

        # Attach parts into message container.
        # msg.attach(part1)
        #msg.attach(part2)

        return msg

    def _sendEmail(self, message):
        try:
            smtpObj = smtplib.SMTP('smtprelay.dlr.de')
            smtpObj.sendmail(self.senderEmail, self.receiverEmail, message)
            smtpObj.quit()
            print('Successfully sent email')

        except smtplib.SMTPException:
            print('Error: unable to send email')

    def _createHTMLTable(self):
        table = []
        table.append("<table style=\"width:100%\">\n")
        table.append("<tr>")
        table.append("<th> Parameter </th>")
        table.append("<th>Value </th>")
        table.append("</tr>")

        for k, v in self.resultsDict.items():
            table.append("\t<tr>\n")
            td = []
            td.append(f"<td>{k}</td>")
            td.append(f"<td>{v}</td>")
            table.append("\t\t" + "".join(td))
            table.append("\n\t</tr>\n")

        table.append("</table>")

        return table

if __name__ == '__main__':
    resDict = {'key1': 12.344, 'key2': 2344.45}

    ResultsEmailGeneration('ulrich.kling@dlr.de', 'ulrich.kling@dlr.de', 'results',
                           resDict)  # , 'C:\\Users\\klin_ul\\Desktop\\tmp\\Bild1.jpg')

