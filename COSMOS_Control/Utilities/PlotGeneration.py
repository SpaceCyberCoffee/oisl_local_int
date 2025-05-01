import io
import matplotlib.pyplot as plt
from base64 import b64encode



def saveFigAsBytesIOObject(xVals, yVals):
    plt.figure()
    plt.plot(xVals, yVals)
    plt.title('Beispiel-Diagramm')

    # Speichern Sie das Diagramm in einem BytesIO-Objekt
    buf = io.BytesIO()
    plt.savefig(buf, format='png')
    buf.seek(0)

    # Base64-codieren Sie das Bild für die Einbettung in HTML
    image_base64 = b64encode(buf.read()).decode('ascii')
    buf.close()
    buf.seek(0)

    return buf

