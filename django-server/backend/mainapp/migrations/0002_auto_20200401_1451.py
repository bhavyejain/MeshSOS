# Generated by Django 3.0.5 on 2020-04-01 14:51

from django.db import migrations


class Migration(migrations.Migration):

    dependencies = [
        ('mainapp', '0001_initial'),
    ]

    operations = [
        migrations.RenameField(
            model_name='request_logs',
            old_name='core_id',
            new_name='emergency',
        ),
        migrations.RemoveField(
            model_name='request_logs',
            name='emergency_type',
        ),
    ]
