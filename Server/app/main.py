from flask import render_template,send_file
from init import app,mqtt,socketio,db
from models import History,Validuser
from datetime import datetime,date
import json
import base64
from PIL import Image
import io

class MyHistory:
   def __init__(self,stt,time,date,name):
      self.stt = stt
      self.time = time
      self.date = date
      self.name  = name

def RecognitionUser():
   import os
   import face_recognition
# Đường dẫn đến thư mục chứa các dataset của các người
   dataset_dir = 'D:/datasetFace'

# Tạo danh sách các mẫu mã khuôn mặt và tên của các người đã biết
   known_face_encodings = []
   known_face_names = []
   for person_dir in os.listdir(dataset_dir):
      person_path = os.path.join(dataset_dir, person_dir)
      person_images = []
      for image_file in os.listdir(person_path):
         image_path = os.path.join(person_path, image_file)
         image = face_recognition.load_image_file(image_path)
         face_encodings = face_recognition.face_encodings(image)
         if len(face_encodings) > 0:
            person_images.append(face_encodings[0])
      if len(person_images) > 0:
         person_encoding = sum(person_images) / len(person_images)
         known_face_encodings.append(person_encoding)
         known_face_names.append(person_dir)

# Load ảnh test và tìm khuôn mặt trong ảnh đó
   test_image_path = 'D:/MyWebApp/Server/app/static/temp_image.jpg'
   test_image = face_recognition.load_image_file(test_image_path)
   face_locations = face_recognition.face_locations(test_image)
   face_encodings = face_recognition.face_encodings(test_image, face_locations)
   name = "Unknown"
# Duyệt qua các khuôn mặt được nhận diện và xác định tên của người đó
   for face_encoding in face_encodings:
      matches = face_recognition.compare_faces(known_face_encodings, face_encoding)
      if True in matches:
         first_match_index = matches.index(True)
         name = known_face_names[first_match_index]
      print(f"Found {name} in the test image.")
   return name

def HandleChartQuery(num,dateInput):
   with app.app_context():
      query = db.select(History).where(History.date == dateInput).order_by(History.stt.desc()).limit(num)
      print("--- QUERY ---\n:",query)
      raw_datalist = db.session.execute(query).scalars().fetchall()
      raw_datalist.reverse()
      print("\n",raw_datalist)
      datalist = []
      for row in raw_datalist:
         datalist.append(MyHistory(stt = row.stt,
                                       time = row.time,
                                       date = row.date,
                                       name = row.name))
         
      json_data = json.dumps([vars(d) for d in datalist])
      print(json_data,'\n')
   return json_data

## Sự kiện khi vừa kết nối MQTT
@mqtt.on_connect()
def handle_connect(client, userdata, flags, rc):
   mqtt.subscribe('door/status')
   mqtt.subscribe('Mytopic/send')
   mqtt.subscribe('InputPwd')
   mqtt.subscribe('thongbao')
   

## Khi nhận message từ MQTT
@mqtt.on_message()
def handle_mqtt_message(client, userdata, message):
   print("nhan duoc tin nhan")
   data = dict(
      topic=message.topic,
      payload=message.payload.decode()
   )
   now = (datetime.now()).strftime("%H:%M:%S")
   datenow = (datetime.now()).strftime("%Y-%m-%d") 
   datastr = data['payload']
   
   ## Topic gửi ảnh đến
   if(data['topic'] == 'Mytopic/send'):
      decoded_data = base64.b64decode(datastr)
      image = Image.open(io.BytesIO(decoded_data))
      image.save('D:/MyWebApp/Server/app/static/temp_image.jpg')
      print("ĐÃ lưu ảnh, Đang thực hiện nhận diện ...... ")
      mqtt.publish("announcetoESP","1")
      socketio.emit('update_image', data = 'temp_image.jpg')
      res_name = RecognitionUser()
      if(res_name != "Unknown"):
         dataobj = History(time = now, date = datenow, method = "nhận diện", name = res_name)
         with app.app_context():
            db.session.add(dataobj)
            db.session.commit()
         print(res_name)
         mqtt.publish('result',str(res_name))
      else:
         mqtt.publish('result',"F")
         print("khong tim thay user")
         
   elif (data['topic'] == 'door/status'):
         print("thong bao mo cua")
         # THONG BAO MO CUA
   elif(data['topic'] == 'InputPwd'):
         print("cap nhat csdl mo khoa = mk")
         dataobj = History(time = now, date = datenow, method = "mật khẩu", name = "Undefined")
   
   elif (data['topic'] == 'thongbao'):
      socketio.emit("updatethongbao",str(data['payload']))
   
      
@socketio.on('connect_homepage')
def get_clientconnect(data):
   print('\ndu lieu nhan duoc:',data)
   socketio.emit('update_on_visited',data = HandleChartQuery(20,(datetime.now()).strftime("%Y-%m-%d")))
   
@socketio.on('date selected')
def get_clientconnect(data):
   print('\ndu lieu nhan duoc:',data)
   socketio.emit('update_on_visited',data = HandleChartQuery(20,str(data)))
   # socketio.emit('update_on_click',data = HandleChartQuery(12))
   
@socketio.on('checkbox value')
def get_clientconnect(data):
   print('\ndu lieu nhan duoc:',data)
   mqtt.publish('control',str(data))
   
@app.route('/')
def home():
   return render_template('home.html')

if __name__ == '__main__':
   socketio.run(app, host='localhost', port=5000, use_reloader=False, debug=True)
