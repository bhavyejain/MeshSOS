from rest_framework import serializers
from . models import request_logs

class rlogSerializer(serializers.ModelSerializer):
    class Meta:
        model = request_logs
        fields = '__all__'