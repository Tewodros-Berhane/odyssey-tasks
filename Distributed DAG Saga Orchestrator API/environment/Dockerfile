FROM python:3.11-slim

ENV PYTHONUNBUFFERED=1 PYTHONDONTWRITEBYTECODE=1 DEBIAN_FRONTEND=noninteractive

WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends sqlite3 curl && rm -rf /var/lib/apt/lists/*

COPY environment/app/requirements.txt /app/requirements.txt
RUN pip install --no-cache-dir -r /app/requirements.txt

COPY environment/app /app
COPY environment/tests /app/tests

RUN chmod -R 755 /app

CMD ["uvicorn", "main:app", "--host", "0.0.0.0", "--port", "8000"]