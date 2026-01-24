from flask import Flask, render_template

app = Flask(__name__)

@app.route('/')
def home():
    with open('datos_lora.log', 'r') as f:
        lines = f.readlines()


    clean_lines = [line.strip() for line in lines]


    return render_template('index.html', data=clean_lines)

if __name__ == '__main__':
    app.run(host='0.0.0.0', debug=True)
