#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(AssistPlayLayer, PlayLayer) {
	struct Fields {
		int holdFrames = 0;               // frames que quedan con el botón pulsado
		CCLabelBMFont* warnLabel = nullptr;
	};

	// Solo ayudamos con el cubo (el resto de modos necesitan otra lógica).
	bool isCubeLike(PlayerObject* p) {
		return !(p->m_isShip || p->m_isBird || p->m_isBall || p->m_isDart ||
		         p->m_isRobot || p->m_isSpider || p->m_isSwing);
	}

	// ¿Hay un obstáculo dentro del recorrido de los próximos `frames` fotogramas?
	bool hazardAhead(int frames) {
		if (!m_objects) return false;

		auto pr = m_player1->getObjectRect();

		// Velocidad horizontal aproximada: 311.58 unidades/s a velocidad 1x.
		float perFrame = m_player1->m_playerSpeed * 311.58f / 60.f;
		float reach = perFrame * static_cast<float>(frames);

		CCRect ahead(pr.origin.x, pr.origin.y, pr.size.width + reach, pr.size.height);

		for (auto obj : CCArrayExt<GameObject*>(m_objects)) {
			if (obj->m_objectType != GameObjectType::Hazard) continue;

			// Descarte rápido por posición para no mirar miles de objetos en detalle.
			float x = obj->getPositionX();
			if (x < pr.origin.x - 30.f || x > pr.getMaxX() + reach + 60.f) continue;

			if (ahead.intersectsRect(obj->getObjectRect())) return true;
		}
		return false;
	}

	void showWarning(bool on) {
		if (!m_fields->warnLabel) {
			auto label = CCLabelBMFont::create("!", "bigFont.fnt");
			auto win = CCDirector::get()->getWinSize();
			label->setPosition({win.width / 2.f, win.height - 40.f});
			label->setScale(0.8f);
			m_uiLayer->addChild(label, 100);
			m_fields->warnLabel = label;
		}
		m_fields->warnLabel->setVisible(on);
	}

	void postUpdate(float dt) {
		PlayLayer::postUpdate(dt);

		auto mod = Mod::get();

		if (!mod->getSettingValue<bool>("enabled")) {
			if (m_fields->warnLabel) m_fields->warnLabel->setVisible(false);
			return;
		}
		if (mod->getSettingValue<bool>("practice-only") && !m_isPracticeMode) {
			if (m_fields->warnLabel) m_fields->warnLabel->setVisible(false);
			return;
		}
		if (!m_player1 || m_player1->m_isDead) return;

		// Soltar el botón tras unos fotogramas de salto.
		if (m_fields->holdFrames > 0) {
			m_fields->holdFrames--;
			if (m_fields->holdFrames == 0) {
				this->releaseButton(PlayerButton::Jump, true);
			}
			return;
		}

		if (!m_player1->m_isOnGround || !isCubeLike(m_player1)) {
			showWarning(false);
			return;
		}

		int frames = static_cast<int>(mod->getSettingValue<int64_t>("lookahead-frames"));
		auto modeName = mod->getSettingValue<std::string>("mode");

		bool danger = hazardAhead(frames);

		if (modeName == "warn") {
			showWarning(danger);
		} else {
			showWarning(false);
			if (danger) {
				this->pushButton(PlayerButton::Jump, true);
				m_fields->holdFrames = 3;
			}
		}
	}
};
