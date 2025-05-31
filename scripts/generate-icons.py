"""
Generate basic icons for WhisperApp
This script creates simple placeholder icons for the application.
"""

import os
from PIL import Image, ImageDraw, ImageFont
import colorsys

def create_app_icon(size=256):
    """Create main application icon with microphone symbol"""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Background circle with gradient effect
    center = size // 2
    radius = int(size * 0.45)
    
    # Draw background circle
    for i in range(radius, 0, -1):
        alpha = int(255 * (i / radius))
        color = (74, 144, 226, alpha)  # Blue gradient
        draw.ellipse([center-i, center-i, center+i, center+i], fill=color)
    
    # Draw microphone shape
    mic_width = size // 8
    mic_height = size // 3
    mic_x = center - mic_width // 2
    mic_y = center - mic_height // 2
    
    # Microphone body (rounded rectangle)
    draw.rounded_rectangle([mic_x, mic_y, mic_x + mic_width, mic_y + mic_height], 
                          radius=mic_width//2, fill='white')
    
    # Microphone stand
    stand_y = mic_y + mic_height
    draw.arc([mic_x - mic_width, stand_y - mic_width//2, 
              mic_x + mic_width * 2, stand_y + mic_height//3], 
             0, 180, fill='white', width=size//40)
    
    # Stand base
    base_y = stand_y + mic_height//4
    draw.line([center, stand_y + mic_height//6, center, base_y], 
              fill='white', width=size//40)
    draw.line([center - mic_width//2, base_y, center + mic_width//2, base_y], 
              fill='white', width=size//40)
    
    return img

def create_action_icon(action, size=32, bg_color=(74, 144, 226)):
    """Create simple action icons"""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Background circle
    margin = 2
    draw.ellipse([margin, margin, size-margin, size-margin], fill=bg_color)
    
    # Draw symbol based on action
    center = size // 2
    symbol_size = size // 3
    
    if action == 'record':
        # Red recording circle
        draw.ellipse([center-symbol_size//2, center-symbol_size//2,
                     center+symbol_size//2, center+symbol_size//2], 
                     fill=(255, 0, 0))
    
    elif action == 'stop':
        # White square
        draw.rectangle([center-symbol_size//2, center-symbol_size//2,
                       center+symbol_size//2, center+symbol_size//2], 
                       fill='white')
    
    elif action == 'pause':
        # Two vertical bars
        bar_width = symbol_size // 4
        draw.rectangle([center-symbol_size//2, center-symbol_size//2,
                       center-symbol_size//2+bar_width, center+symbol_size//2], 
                       fill='white')
        draw.rectangle([center+symbol_size//2-bar_width, center-symbol_size//2,
                       center+symbol_size//2, center+symbol_size//2], 
                       fill='white')
    
    elif action == 'play':
        # Triangle pointing right
        points = [(center-symbol_size//2, center-symbol_size//2),
                  (center-symbol_size//2, center+symbol_size//2),
                  (center+symbol_size//2, center)]
        draw.polygon(points, fill='white')
    
    elif action == 'settings':
        # Gear symbol (simplified)
        draw.ellipse([center-symbol_size//2, center-symbol_size//2,
                     center+symbol_size//2, center+symbol_size//2], 
                     fill='white')
        draw.ellipse([center-symbol_size//3, center-symbol_size//3,
                     center+symbol_size//3, center+symbol_size//3], 
                     fill=bg_color)
    
    elif action == 'microphone':
        # Small microphone icon
        mic_width = symbol_size // 3
        mic_height = symbol_size
        draw.rounded_rectangle([center-mic_width//2, center-mic_height//2,
                               center+mic_width//2, center+mic_height//4], 
                               radius=mic_width//2, fill='white')
    
    return img

def create_status_icon(status, size=16):
    """Create status indicator icons"""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    colors = {
        'online': (0, 255, 0),      # Green
        'offline': (128, 128, 128), # Gray
        'error': (255, 0, 0),       # Red
        'warning': (255, 165, 0),   # Orange
        'info': (0, 123, 255),      # Blue
        'success': (0, 255, 0),     # Green
        'downloading': (255, 165, 0) # Orange
    }
    
    color = colors.get(status, (128, 128, 128))
    draw.ellipse([1, 1, size-1, size-1], fill=color)
    
    return img

def main():
    """Generate all required icons"""
    icons_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'resources', 'icons')
    os.makedirs(icons_dir, exist_ok=True)
    
    print("Generating WhisperApp icons...")
    
    # Main app icons in various sizes
    sizes = [16, 32, 64, 128, 256]
    for size in sizes:
        icon = create_app_icon(size)
        icon.save(os.path.join(icons_dir, f'app-{size}.png'))
        print(f"Created app-{size}.png")
    
    # Save main app.png
    main_icon = create_app_icon(256)
    main_icon.save(os.path.join(icons_dir, 'app.png'))
    print("Created app.png")
    
    # Convert to ICO format (Windows icon)
    icon_sizes = [(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
    app_ico = Image.new('RGBA', (256, 256), (0, 0, 0, 0))
    app_ico.paste(main_icon, (0, 0))
    app_ico.save(os.path.join(icons_dir, 'app.ico'), format='ICO', sizes=icon_sizes)
    print("Created app.ico")
    
    # Action icons
    actions = ['record', 'stop', 'pause', 'play', 'settings', 'microphone']
    for action in actions:
        icon = create_action_icon(action)
        icon.save(os.path.join(icons_dir, f'{action}.png'))
        print(f"Created {action}.png")
    
    # Status icons
    statuses = ['online', 'offline', 'error', 'warning', 'info', 'success', 'downloading']
    for status in statuses:
        icon = create_status_icon(status)
        icon.save(os.path.join(icons_dir, f'{status}.png'))
        print(f"Created {status}.png")
    
    # Create placeholder icons for other required icons
    placeholder_icons = [
        'help', 'about', 'exit', 'new', 'open', 'save', 'save-as', 'export',
        'cut', 'copy', 'paste', 'undo', 'redo', 'find', 'replace',
        'zoom-in', 'zoom-out', 'zoom-reset', 'fullscreen',
        'speaker', 'headphones', 'model', 'download', 'delete', 'refresh',
        'flag-us', 'flag-es', 'flag-fr', 'flag-de', 'flag-cn', 'flag-jp'
    ]
    
    for icon_name in placeholder_icons:
        # Create simple placeholder with first letter
        img = Image.new('RGBA', (32, 32), (0, 0, 0, 0))
        draw = ImageDraw.Draw(img)
        draw.ellipse([2, 2, 30, 30], fill=(74, 144, 226))
        
        # Add first letter
        try:
            font = ImageFont.truetype("arial.ttf", 16)
        except:
            font = ImageFont.load_default()
        
        letter = icon_name[0].upper()
        if icon_name.startswith('flag-'):
            letter = icon_name.split('-')[1].upper()[:2]
        
        draw.text((16, 16), letter, fill='white', font=font, anchor='mm')
        img.save(os.path.join(icons_dir, f'{icon_name}.png'))
        print(f"Created {icon_name}.png")
    
    print("\nAll icons generated successfully!")
    print(f"Icons saved to: {icons_dir}")

if __name__ == "__main__":
    main()