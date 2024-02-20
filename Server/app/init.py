from flask import Flask
from flask_mqtt import Mqtt
from flask_socketio import SocketIO
from flask_sqlalchemy import SQLAlchemy
import eventlet

eventlet.monkey_patch()

app = Flask(__name__)

app.config['SQLALCHEMY_DATABASE_URI'] = 'mysql+pymysql://root:1475369Minh@localhost/access?charset=utf8mb4'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = True
app.config['SECRET_KEY'] = 'helloworld'

db = SQLAlchemy(app)

app.app_context().push()

from flask_mqtt import Mqtt

app.config['SECRET_KEY'] = 'helloworld'
app.config['MQTT_CLIENT_ID'] = 'FlaskClient'
app.config['MQTT_BROKER_URL'] = 'nightkoala963.cloud.shiftr.io'
app.config['MQTT_BROKER_PORT'] = 1883
app.config['MQTT_USERNAME'] = 'nightkoala963'
app.config['MQTT_PASSWORD'] = 'ijfMV2BbIpZgvPBG'

mqtt = Mqtt(app)


from flask_socketio import SocketIO
socketio = SocketIO(app)



