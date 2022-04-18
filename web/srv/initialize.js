/*
Copyright Glen Knowles 2022.
Distributed under the Boost Software License, Version 1.0.
*/

var loadWaits = 1

//===========================================================================
function includeHtmlFragment(src) {
  let node = document.currentScript
  loadWaits += 1
  const frame = document.createElement("iframe")
  node.insertAdjacentElement('afterend', frame)
  frame.style.display = 'none'
  frame.onload = function() {
    this.insertAdjacentHTML('afterend', this.contentDocument.body.innerHTML)
    this.remove()
    createApp()
  }
  frame.src = src
  node.remove()
}

//===========================================================================
function createApp() {
  loadWaits -= 1
  if (loadWaits > 0) 
    return

  const app = Vue.createApp({
      data() {
          return srvdata
      },
      computed: {
          nowSecs() {
              return Date.parse(this.now) / 1000
          },
          appName() { 
              let out = this.server.baseName
              if (this.server.appIndex > 1)
                  out += this.server.appIndex.toString()
              return out
          },
      },
      methods: {
          readableDuration,
          elapsedTime,
      },
  });

  app.mount('#app')
  document.getElementById('app').style.visibility = 'visible'
}

//===========================================================================
function addTags(tags) {
    let self = document.currentScript
    for (let i = tags.length - 1; i >= 0; --i) {
        let el = document.createElement(tags[i].tag)
        for (let prop in tags[i].props) {
            el[prop] = tags[i].props[prop]
        }
        self.insertAdjacentElement('afterend', el)
    }
}

//===========================================================================
function finalize() {
    let tags = [
        { tag: 'script', props: {
            src: 'https://cdn.jsdelivr.net/npm/@popperjs/core@2.10.2/dist/umd/popper.min.js',
            integrity: 'sha384-7+zCNj/IqJ95wo16oMtfsKbZ9ccEh31eOz1HGyDuCQ6wgnyJNSYdrPa03rtR1zdB',
            crossOrigin: 'anonymous',
        }},
        { tag: 'script', props: {
            src: 'https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/js/bootstrap.min.js',
            integrity: 'sha384-QJHtvGhmr9XOIpI6YVutG+2QOK9T+ZnN4kzFN1RtK3zEFEIsxhlmWl5/YESvpZ13',
            crossOrigin: 'anonymous',
        }},
    ]

    addTags(tags)
}


/****************************************************************************
*
*   Main
*
***/

//===========================================================================
function init() {
    let tags = [
        { tag: 'link', props: { 
            rel: 'stylesheet', 
            integrity: 'sha384-1BmE4kWBq78iYhFldvKuhfTAU6auU8tT94WrHftjDbrCEXSU1oBoqyl2QvZ6jIW3',
            crossOrigin: 'anonymous',
            href: 'https://cdn.jsdelivr.net/npm/bootstrap@5.1.3/dist/css/bootstrap.min.css',
        }},
        { tag: 'link', props: {
            rel: 'stylesheet', 
            href: 'https://cdn.jsdelivr.net/npm/bootstrap-icons@1.8.1/font/bootstrap-icons.css',
        }},
        { tag: 'script', props: {
            src: 'https://unpkg.com/vue@3',
        }},
        { tag: 'script', props: {
            src: 'util.js',
        }},
    ]

    addTags(tags)
    window.addEventListener('load', createApp)
}

init()
