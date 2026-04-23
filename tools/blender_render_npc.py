from PIL import Image
import os

# Шляхи до файлів
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
INPUT_PATH = os.path.join(ROOT, "assets", "hands.png")
OUTPUT_PATH = os.path.join(ROOT, "assets", "hands_clean.png")

def remove_background(target_color=(255, 255, 255)): # За замовчуванням білий
    if not os.path.exists(INPUT_PATH):
        print(f"[Помилка] Файл не знайдено: {INPUT_PATH}")
        return

    img = Image.open(INPUT_PATH).convert("RGBA")
    datas = img.getdata()

    new_data = []
    for item in datas:
        # Якщо колір пікселя збігається з фоновим (білим), робимо його прозорим
        # item[0,1,2] це R, G, B
        if item[0] > 240 and item[1] > 240 and item[2] > 240: 
            new_data.append((255, 255, 255, 0)) # Прозорий піксель
        else:
            new_data.append(item)

    img.putdata(new_data)
    img.save(OUTPUT_PATH, "PNG")
    print(f"[OK] Фон видалено! Збережено у: {OUTPUT_PATH}")

if __name__ == "__main__":
    remove_background()