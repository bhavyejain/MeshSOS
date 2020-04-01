from django.shortcuts import render

from django.http import HttpResponse
from django.shortcuts import get_object_or_404
from rest_framework.views import APIView
from rest_framework.response import Response
from rest_framework import status

from . models import request_logs
from . serializers import rlogSerializer

import json

class rloglist(APIView):
    def get(self, request):
        rloglist = request_logs.objects.all()
        serializer = rlogSerializer(rloglist, many = True)
        return Response(serializer.data)

    def post(self, request):
        # dictionary
        log_data = request.data 

        serializer = rlogSerializer(data=log_data)

        if serializer.is_valid(raise_exception=True):
            saved_obj = serializer.save()

        return Response("data", )