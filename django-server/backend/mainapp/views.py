from django.shortcuts import render

from django.http import HttpResponse
from django.shortcuts import get_object_or_404
from rest_framework.views import APIView
from rest_framework.response import Response
from rest_framework import status

from . models import request_logs
from . serializers import rlogSerializer

from datetime import datetime
import pytz

class rloglist(APIView):
    def get(self, request):
        rloglist = request_logs.objects.all()
        serializer = rlogSerializer(rloglist, many = True)
        return Response(serializer.data)

    def post(self, request):
        # dictionary of recieved data body
        req_data = request.data 

        print("request_data:\n:", req_data)
        
        # update timestamp to use indian time (UTC -> Asia/Kolkata)
        utc_datetime = datetime.strptime(req_data['timestamp'], '%Y-%m-%dT%H:%M:%S.%fZ')
        utc_datetime = utc_datetime.replace(tzinfo=pytz.utc)

        localFormat = "%Y-%m-%d %H:%M:%S"
        localTimeZone = pytz.timezone('Asia/Kolkata')

        localDatetime = utc_datetime.astimezone(localTimeZone)
            
        req_data.update(timestamp = localDatetime.strftime(localFormat))

        # divide emergency string in emergency_type, core_id
        emergency = req_data['emergency']
        emergency_type, core_id = emergency.split('-')

        del req_data['emergency']
        req_data.update[('emergency_type', emergency_type),('core_id', core_id)]

        # serialize data to save in db
        serializer = rlogSerializer(data=req_data)

        if serializer.is_valid(raise_exception=True):
            saved_obj = serializer.save()

        # return_val
        if req_data['latitude'] == "-1" or req_data['longitude'] == "-1" or req_data['accuracy'] == "-1":
            return_val = emergency+"/0"
        else:
            return_val = emergency+"/1"
        
        return Response(return_val, )