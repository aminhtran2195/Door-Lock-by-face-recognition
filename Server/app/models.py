from init import db
from sqlalchemy import Column,Integer,String,Float,DateTime

class History(db.Model):
    __tablename__ = 'history'
    stt = Column(Integer,primary_key=True, autoincrement=True)
    time = Column(String(45),nullable=False)
    date = Column(String(45),nullable=False)
    method = Column(String(45),nullable=False)
    name = Column(String(45),nullable=True) 
     
class Validuser(db.Model):
    __tablename__ = 'validuser'
    stt = Column(Integer,primary_key=True, autoincrement=True)
    name = Column(String(45),nullable=False) 
    
if __name__ == '__main__':
    db.create_all()