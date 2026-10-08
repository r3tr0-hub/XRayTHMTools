# Публикация отдельного продукта

1. Создайте пустой репозиторий https://github.com/new с владельцем r3tr0-hub и именем XRayTHMTools. Не добавляйте README, лицензию или .gitignore на сайте.
2. Распакуйте этот проект в новую папку. Откройте Git Bash там, где находится CMakeLists.txt.
3. Выполните:

```bash
git init
git config user.name "r3tr0-hub"
git config user.email "ВАША_ПОЧТА_GITHUB"
git add .
git status
git commit -m "Initial release: XRayTHMTools v0.4.0-beta"
git branch -M main
git remote add origin https://github.com/r3tr0-hub/XRayTHMTools.git
git push -u origin main
```

Почту замените своей или noreply-адресом из настроек GitHub. В git status не должно быть папок сборки и бинарных файлов.
4. Откройте Actions нового репозитория и дождитесь успешной сборки. Скачайте artifact XRayTHMTools-v0.4.0-beta и распакуйте его.
5. Проверьте готовый внутренний ZIP на Windows: установка, F3, Esc после щелчков по отчёту/кнопкам, F4, сохранение копии THM и повторное чтение.
6. Откройте https://github.com/r3tr0-hub/XRayTHMTools/releases/new : тег v0.4.0-beta, Target main, название X-Ray THM Tools v0.4.0-beta. Описание — RELEASE_NOTES_RU.md.
7. Прикрепите внутренний XRayTHMTools-v0.4.0-beta.zip и XRayTHMTools-v0.4.0-beta.zip.sha256. Отметьте pre-release и опубликуйте.

Описание репозитория (About):
Плагин Total Commander и редактор текстурных THM для S.T.A.L.K.E.R. X-Ray SDK. Просмотр по F3, редактирование, импорт параметров DDS, резервные копии. Windows x86/x64.
